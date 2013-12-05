/**
 * @brief Contains the entry part of the HLDR Detector ANaConDA plugin.
 *
 * A file containing the entry part of the High-Level Data Race (HLDR) Detector
 *   ANaConDA plugin.
 *
 * @file      hldr-detector.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-11-21
 * @date      Last Update 2013-12-05
 * @version   0.5.1
 */

#include "anaconda.h"

#include <assert.h>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <list>
#include <set>
//#include <vector>

#include "utils/scopedlock.hpp"

#define VIEW_HISTORY_WINDOW_SIZE 5

/**
 * @brief A structure representing a view, i.e., a set of memory accesses which
 *   are performed in an atomic region (critical section, transaction, etc.).
 */
typedef struct View_s
{
  typedef std::set< ADDRINT > ContainerType;
  typedef ContainerType::iterator Iterator;
  ContainerType reads; //!< Reads performed in an atomic region.
  ContainerType writes; //!< Writes performed in an atomic region.
  /**
   * @brief A number of threads which are currently referencing this view. When
   *   this number drops to zero, the view can be safely removed if not needed.
   *
   * @note This counter is also used to track the number of atomic regions we
   *   are in which is needed in order to support nested atomic regions. Note
   *   that it is safe to use the counter for this purpose, because this info
   *   is needed only during the construction of the view when other threads
   *   cannot reference it. After the view is inserted into the history (and
   *   other threads may reference it), the counter is already used to track
   *   the number of threads which reference the view.
   */
  std::atomic< int > refs;
} View;

class ViewHistory
{
  public: // Type definitions
    typedef std::list< View* > ContainerType;
    typedef ContainerType::iterator Iterator;
    typedef struct Window_s
    {
      Iterator first;
      Iterator last;
      bool empty;

      Window_s() : first(), last(), empty(true) {}
    } Window;
  private:
    ContainerType m_views;
    size_t m_windowSize;
    Window m_window;
    PIN_RWMUTEX m_windowLock;
  public:
    ViewHistory(size_t ws) : m_windowSize(ws)
    {
      m_window.first = m_views.end();
      m_window.last = m_views.end();

      PIN_RWMutexInit(&m_windowLock);
    }

  public:
    ~ViewHistory()
    {
      PIN_RWMutexFini(&m_windowLock);
    }

  public:
    void insert(View* view)
    {
      ScopedWriteLock lock(m_windowLock);

      m_window.first = m_views.insert(m_window.first, view);

      if (m_views.size() > m_windowSize)
      {
        m_window.last--;
      }
      else if (m_window.empty)
      {
        m_window.last = m_window.first;
        m_window.empty = false;
      }
    }

    Window acquire()
    {
      ScopedReadLock lock(m_windowLock);

      if (!m_window.empty)
        (*m_window.last)->refs++; // Atomic

      return m_window;
    }

    void release(Window window)
    {
      if (!window.empty)
        (*window.last)->refs--; // Atomic
    }

  public:
    void print(std::ostream& s = std::cout)
    {
      std::string output = "View History Begin\n";

      for (Iterator it = m_views.begin(); it != m_views.end(); it++)
      {
        if (it == m_window.first || it == m_window.last)
          output += "-> ";
        else
          output += "   ";

        output += "View[refs=" + decstr((*it)->refs) + ",accesses=(";

        for (View::Iterator vit = (*it)->reads.begin(); vit != (*it)->reads.end(); vit++)
        {
          output += "R:" + hexstr(*vit) + ",";
        }
        for (View::Iterator vit = (*it)->writes.begin(); vit != (*it)->writes.end(); vit++)
        {
          output += "W:" + hexstr(*vit) + ",";
        }

        if (output[output.size() - 1] == ',')
          output[output.size() - 1] = ')';
        else
          output += ")";

        output += "]\n";
      }

      output += "View History End\n";

      s << output;
    }
};

namespace
{ // Static global variables (usable only within this module)
  VOID freeCurrentView(VOID* data) { /* Should be already freed when called */ };
  VOID freeViewHistory(VOID* data) { delete static_cast< ViewHistory* >(data); };

  TLS_KEY g_currentViewTlsKey = TLS_CreateThreadDataKey(freeCurrentView);
  TLS_KEY g_viewHistoryTlsKey = TLS_CreateThreadDataKey(freeViewHistory);

  std::list< THREADID > g_threads;
  PIN_RWMUTEX g_threadsLock;
}

#define VIEW static_cast< View* >(TLS_GetThreadData(g_currentViewTlsKey, tid))
#define VIEW_HISTORY static_cast< ViewHistory* >(TLS_GetThreadData(g_viewHistoryTlsKey, tid))
#define REMOTE_VIEW_HISTORY(tid) static_cast< ViewHistory* >(TLS_GetThreadData(g_viewHistoryTlsKey, tid))

typedef std::vector< View::ContainerType > Views;
typedef const View::ContainerType& (*GETACCESSESFUNPTR)(const View* view);

inline
const View::ContainerType& writes(const View* view)
{
  return view->writes;
}

inline
const View::ContainerType& reads(const View* view)
{
  return view->reads;
}

Views intersect(const View::ContainerType& view, ViewHistory::Window window, GETACCESSESFUNPTR accesses)
{
  Views views;

  ViewHistory::Iterator it = window.first;

  do
  {
    View::ContainerType vp;

    std::set_intersection(view.begin(), view.end(),
      accesses(*it).begin(), accesses(*it).end(),
      std::inserter(vp, vp.begin()));

    views.push_back(vp);
  } while (it++ != window.last);

  return views;
}

bool formChain(Views views)
{
  for (int i = 1; i < views.size(); i++)
  {
    View::ContainerType si;

    std::set_intersection(
      views[i-1].begin(), views[i-1].end(),
      views[i].begin(), views[i].end(),
      std::inserter(si, si.begin()));

    if (si.size() != views[i-1].size() && si.size() != views[i].size())
      return false;
  }

  return true;
}

bool containsHldr(View* view, ViewHistory::Window window)
{
  if (!formChain(intersect(view->writes, window, writes)))
  {
    return true;
  }
  else if (!formChain(intersect(view->writes, window, reads)))
  {
    return true;
  }
  else if (!formChain(intersect(view->reads, window, writes)))
  {
    return true;
  }

  return false;
}

bool checkThisViewAgainstOtherHistories(THREADID tid, View* view)
{
  ScopedReadLock lock(g_threadsLock);

  for (std::list< THREADID >::iterator it = g_threads.begin();
    it != g_threads.end(); it++)
  {
    if (*it != tid)
    {
      ViewHistory* history = REMOTE_VIEW_HISTORY(*it);

      ViewHistory::Window window = history->acquire();

      if (window.empty)
      {
        history->release(window);

        continue;
      }

      if (containsHldr(view, window))
      {
        history->release(window);

        return true;
      }

      history->release(window);
    }
  }

  return false;
}

bool checkOtherViewsAgainstThisHistory(THREADID tid)
{
  ScopedReadLock lock(g_threadsLock);

  ViewHistory::Window window = VIEW_HISTORY->acquire();

  for (std::list< THREADID >::iterator it = g_threads.begin();
    it != g_threads.end(); it++)
  {
    if (*it != tid)
    {
      ViewHistory* history = REMOTE_VIEW_HISTORY(*it);

      ViewHistory::Window views = history->acquire();

      if (views.empty)
      {
        history->release(views);

        continue;
      }

      ViewHistory::Iterator it = views.first;

      do
      {
        if (containsHldr(*it, window))
        {
          history->release(views);

          VIEW_HISTORY->release(window);

          return true;
        }
      } while (it++ != views.last);

      history->release(views);
    }
  }

  VIEW_HISTORY->release(window);

  return false;
}

/**
 * Creates a new view.
 *
 * @note This function is called when a thread enters an atomic region.
 *
 * @param tid A number uniquely identifying a thread entering an atomic region.
 */
inline
VOID atomicRegionEntered(THREADID tid)
{
  if (VIEW == NULL)
  { // Not entering a nested atomic region, need to create a new view
    TLS_SetThreadData(g_currentViewTlsKey, new View(), tid);
  }

  VIEW->refs++; // The number of atomic regions we are in
}

/**
 * Update the current view.
 *
 * @note This function is called when a thread reads from a memory.
 *
 * @param tid A number uniquely identifying a thread reading from the memory.
 * @param addr An address from which the thread read some data.
 */
inline
VOID memoryRead(THREADID tid, ADDRINT addr)
{
  if (VIEW != NULL)
  { // We are in an atomic region
    VIEW->reads.insert(addr);
  }
}

/**
 * Updates the current view.
 *
 * @note This function is called when a thread reads from a memory.
 *
 * @param tid A number uniquely identifying a thread writing to the memory.
 * @param addr An address to which the thread written some data.
 */
inline
VOID memoryWritten(THREADID tid, ADDRINT addr)
{
  if (VIEW != NULL)
  { // We are in an atomic region
    VIEW->writes.insert(addr);
  }
}

/**
 * Saves the current view and checks it against the views of other threads to
 *   determine if a High-Level Data Race is possible.
 *
 * @note This function is called when a thread exits an atomic region.
 *
 * @param tid A number uniquely identifying a thread exiting an atomic region.
 */
inline
VOID atomicRegionExited(THREADID tid)
{
  VIEW->refs--; // The number of atomic regions we are in

  if (VIEW->refs > 0) return; // We are still in some atomic region

  // First check the current (new) view against the views of other threads
  if (checkThisViewAgainstOtherHistories(tid, VIEW))
    CONSOLE("Found HLDR!\n");

  // Then save the current (new) view to the view history
  VIEW_HISTORY->insert(VIEW);

  // Finally, check the views of other threads against this thread's views
  if (checkOtherViewsAgainstThisHistory(tid))
    CONSOLE("Found HLDR!\n");

  // At last, clear the current view as we are leaving an atomic region
  TLS_SetThreadData(g_currentViewTlsKey, NULL, tid);
}

VOID threadStarted(THREADID tid)
{
  TLS_SetThreadData(g_viewHistoryTlsKey, new ViewHistory(VIEW_HISTORY_WINDOW_SIZE), tid);

  g_threads.push_back(tid);
}

VOID threadFinished(THREADID tid)
{
  ScopedWriteLock lock(g_threadsLock);

  g_threads.remove(tid);

  VIEW_HISTORY->print();
}

VOID beforeTxStart(THREADID tid)
{
  //
}

VOID afterTxStart(THREADID tid, ADDRINT* result)
{
  atomicRegionEntered(tid);
}

VOID beforeTxCommit(THREADID tid)
{
  //
}

VOID afterTxCommit(THREADID tid, ADDRINT* result)
{
  if (result != NULL && *result == 1)
  { // The commit was successful, so we are leaving the atomic region now
    atomicRegionExited(tid);
  }
}

VOID beforeTxAbort(THREADID tid)
{
  //
}

VOID afterTxAbort(THREADID tid, ADDRINT* result)
{
  //
}

VOID beforeTxRead(THREADID tid, ADDRINT addr)
{
  memoryRead(tid, addr);
}

VOID afterTxRead(THREADID tid, ADDRINT addr)
{
  //
}

VOID beforeTxWrite(THREADID tid, ADDRINT addr)
{
  memoryWritten(tid, addr);
}

VOID afterTxWrite(THREADID tid, ADDRINT addr)
{
  //
}

VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  atomicRegionEntered(tid);
}

VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  atomicRegionExited(tid);
}

VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, BOOL isLocal)
{
  if (isLocal) return; // Ignore local variables

  memoryRead(tid, addr);
}

VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, BOOL isLocal)
{
  if (isLocal) return; // Ignore local variables

  memoryWritten(tid, addr);
}

/**
 * Initialises the HLDR detector plugin.
 */
PLUGIN_INIT_FUNCTION()
{
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);

  TM_BeforeTxStart(beforeTxStart);
  TM_BeforeTxCommit(beforeTxCommit);
  TM_BeforeTxAbort(beforeTxAbort);
  TM_BeforeTxRead(beforeTxRead);
  TM_BeforeTxWrite(beforeTxWrite);

  TM_AfterTxStart(afterTxStart);
  TM_AfterTxCommit(afterTxCommit);
  TM_AfterTxAbort(afterTxAbort);
  TM_AfterTxRead(afterTxRead);
  TM_AfterTxWrite(afterTxWrite);

  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);

  ACCESS_BeforeMemoryRead(beforeMemoryRead);
  ACCESS_BeforeMemoryWrite(beforeMemoryWrite);

  PIN_RWMutexInit(&g_threadsLock);
}

PLUGIN_FINISH_FUNCTION()
{
  PIN_RWMutexFini(&g_threadsLock);
}

/** End of file hldr-detector.cpp **/
