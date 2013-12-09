/**
 * @brief Contains the entry part of the HLDR Detector ANaConDA plugin.
 *
 * A file containing the entry part of the High-Level Data Race (HLDR) Detector
 *   ANaConDA plugin.
 *
 * @file      hldr-detector.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-11-21
 * @date      Last Update 2013-12-09
 * @version   0.6
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

// Helper macros
#define VIEW static_cast< View* >(TLS_GetThreadData(g_currentViewTlsKey, tid))
#define VIEW_HISTORY(tid) static_cast< ViewHistory* >(TLS_GetThreadData(g_viewHistoryTlsKey, tid))

/**
 * @brief Simplifies acquisition of ViewHistory's windows.
 *
 * A helper class simplifying thread-safe acquisition of ViewHistory's windows.
 *   When an object of this class is created, it acquires the current window of
 *   a view history of some thread. When the object is deleted, is releases the
 *   window automatically.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-12-09
 * @date      Last Update 2013-12-09
 * @version   0.1
 */
class LockedWindow
{
  private: // Internal variables
    ViewHistory* m_history; //!< A view history of a thread.
    ViewHistory::Window m_window; //!< A window acquired by the class.
  public: // Constructors
    /**
     * Constructs a LockedWindow object using a specific view history.
     *
     * @param history A view history whose window should be acquired.
     */
    LockedWindow(ViewHistory* history) : m_history(history)
    {
      m_window = m_history->acquire();
    }

    /**
     * Constructs a LockedWindow object using view history of a specific thread.
     *
     * @param tid A number uniquely identifying a thread whose window should be
     *   acquired.
     */
    LockedWindow(THREADID tid) : m_history(VIEW_HISTORY(tid))
    {
      m_window = m_history->acquire();
    }

  public: // Destructors
    /**
     * Destroys a LockedWindow object.
     */
    ~LockedWindow()
    {
      m_history->release(m_window);
    }

  public: // Member methods
    /**
     * Checks if the acquired window is empty.
     *
     * @return @em True if the acquired window is empty, @em false otherwise.
     */
    bool empty() { return m_window.empty; }

    /**
     * Converts a LockedWindow object to a normal Window object.
     */
    operator ViewHistory::Window() { return m_window; }
};

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

/**
 * Checks if a sequence of sets of elements forms a chain.
 *
 * @tparam T A container holding a set of elements in a sequence.
 * @tparam S A sequence supporting direct access to its elements using the []
 *   operator. If no sequence is specified, std::vector will be used.
 *
 * @param seq A sequence of sets of elements to be checked.
 * @param cvp A pair of sets of elements violating the chain. Given as indexes
 *   to these elements. Set only if the sequence does not form a chain.
 * @return @em True if the sequence forms a chain, @em false otherwise.
 */
template< typename T, template< class T, class = std::allocator< T > >
  class S = std::vector >
bool isChain(const S< T >& seq, std::pair< int, int >& cvp)
{
  for (int i = 0; i < seq.size(); i++)
  { // For every two sets of elements, check if they do not violate the chain
    for (int j = i + 1; j < seq.size(); j++)
    { // Commutative operation, check(seq[i], seq[j]) == check(seq[j], seq[i])
      T si;

      // Compute the intersection of the current sets, si = seq[i] \cap seq[j]
      std::set_intersection(seq[i].begin(), seq[i].end(), seq[j].begin(),
        seq[j].end(), std::inserter(si, si.begin()));

      // Check if the sets violate the chain, si != seq[i] && si != seq[j]
      if (si.size() != seq[i].size() && si.size() != seq[j].size())
      { // Checking size is sufficient here, as we compare two sets with their
        // intersection, if the size is not equal, the sets are also not equal
        cvp.first = i;
        cvp.second = j;

        return false; // The sequence does not form a chain
      }
    }
  }

  return true; // The sequence forms a chain, no violation detected
}

bool containsHldr(View* view, ViewHistory::Window window)
{
  std::pair< int, int > cvp;

  if (!isChain(intersect(view->writes, window, writes), cvp))
  {
    return true;
  }
  else if (!isChain(intersect(view->writes, window, reads), cvp))
  {
    return true;
  }
  else if (!isChain(intersect(view->reads, window, writes), cvp))
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
      LockedWindow window(tid);

      if (window.empty()) continue;

      if (containsHldr(view, window)) return true;
    }
  }

  return false;
}

bool checkOtherViewsAgainstThisHistory(THREADID tid)
{
  ScopedReadLock lock(g_threadsLock);

  LockedWindow window(tid);

  for (std::list< THREADID >::iterator it = g_threads.begin();
    it != g_threads.end(); it++)
  {
    if (*it != tid)
    {
      LockedWindow views(*it);

      if (views.empty()) continue;

      ViewHistory::Iterator it = ((ViewHistory::Window)views).first;

      do
      {
        if (containsHldr(*it, window)) return true;
      } while (it++ != ((ViewHistory::Window)views).last);
    }
  }

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
  VIEW_HISTORY(tid)->insert(VIEW);

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

  VIEW_HISTORY(tid)->print();
}

VOID afterTxStart(THREADID tid, ADDRINT* result)
{
  atomicRegionEntered(tid);
}

VOID afterTxCommit(THREADID tid, ADDRINT* result)
{
  if (result != NULL && *result == 1)
  { // The commit was successful, so we are leaving the atomic region now
    atomicRegionExited(tid);
  }
}

VOID beforeTxRead(THREADID tid, ADDRINT addr)
{
  memoryRead(tid, addr);
}

VOID beforeTxWrite(THREADID tid, ADDRINT addr)
{
  memoryWritten(tid, addr);
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
  // Functions for thread initialisation and cleanup
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);

  // Functions for monitoring atomic regions represented by transactions
  TM_AfterTxStart(afterTxStart);
  TM_AfterTxCommit(afterTxCommit);

  // Functions for monitoring accesses in atomic regions (transactions)
  TM_BeforeTxRead(beforeTxRead);
  TM_BeforeTxWrite(beforeTxWrite);

  // Functions for monitoring atomic regions represented by critical sections
  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);

  // Functions for monitoring accesses in atomic regions (critical sections)
  ACCESS_BeforeMemoryRead(beforeMemoryRead);
  ACCESS_BeforeMemoryWrite(beforeMemoryWrite);

  // Prepare a lock for synchronising accesses to the list of running threads
  PIN_RWMutexInit(&g_threadsLock);
}

/**
 * Cleans up the HLDR detector plugin.
 */
PLUGIN_FINISH_FUNCTION()
{
  // Free the lock for synchronising accesses to the list of running threads
  PIN_RWMutexFini(&g_threadsLock);
}

/** End of file hldr-detector.cpp **/
