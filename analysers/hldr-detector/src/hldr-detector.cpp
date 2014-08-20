/**
 * @brief Contains the entry part of the HLDR Detector ANaConDA plugin.
 *
 * A file containing the entry part of the High-Level Data Race (HLDR) Detector
 *   ANaConDA plugin.
 *
 * @file      hldr-detector.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-11-21
 * @date      Last Update 2014-01-27
 * @version   0.9.6
 */

#include "anaconda.h"

#include <assert.h>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "utils/scopedlock.hpp"

#define VIEW_HISTORY_WINDOW_SIZE 5

// Type definitions
typedef unsigned long timestamp_t;

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
   * @brief Instructions reading from a specific memory address.
   */
  std::map< ADDRINT, ContainerType > ris;
  /**
   * @brief Instructions writing to a specific memory address.
   */
  std::map< ADDRINT, ContainerType > wis;
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
  timestamp_t timestamp; //!< A timestamp of the time the view was completed.
  Backtrace startbt; //!< A backtrace at the start of an atomic region.
  Backtrace endbt; //!< A backtrace at the end of an atomic region.
  std::atomic< int > depth; //!< A number of nested atomic regions encountered.
} View;

/**
 * Concatenates a string with a set of memory addresses.
 *
 * @param s A string.
 * @param set A set of memory addresses.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em set.
 */
inline
std::string operator+(const std::string& s, const View::ContainerType& set)
{
  std::string tmp = "["; // Put all items in the set into square brackets

  for (typename View::Iterator it = set.begin(); it != set.end(); it++)
  { // Append all items in the set
    tmp += hexstr(*it) + ",";
  }

  // Close the square brackets
  if (tmp[tmp.size() - 1] == ',') tmp[tmp.size() - 1] = ']'; else tmp += "]";

  return s + tmp; // Return the concatenation of the string and set items
}

/**
 * Concatenates a string with a view object.
 *
 * @param s A string.
 * @param view A view object.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em view.
 */
inline
std::string operator+(const std::string& s, const View* view)
{
  return s + "View(timestamp=" + decstr(view->timestamp) + ",refs="
    + decstr(view->refs) + ",reads=" + view->reads + ",writes="
    + view->writes + ")";
}

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
          output += std::string("-> ") + *it + "\n";
        else
          output += std::string("   ") + *it + "\n";
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

  // Type definitions
  typedef std::list< THREADID > ThreadContainerType;
  typedef ThreadContainerType::iterator ThreadIterator;

  ThreadContainerType g_threads; //!< A list of currently running threads.
  PIN_RWMUTEX g_threadsLock; //!< A lock guarding access to @c g_threads.
  std::atomic< timestamp_t > g_clock; //!< A logical clock.
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
 * @version   0.1.1
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

  public: // An interface for accessing the acquired window
    /**
     * Gets an iterator pointing to the first view in the acquired window.
     *
     * @return An iterator pointing to the first view in the acquired window.
     */
    ViewHistory::Iterator first() { return m_window.first; }

    /**
     * Gets an iterator pointing to the last view in the acquired window.
     *
     * @return An iterator pointing to the last view in the acquired window.
     */
    ViewHistory::Iterator last() { return m_window.last; }

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

/**
 * Gets all write accesses contained in a view.
 *
 * @param view A view.
 * @return A list of write accesses contained in the view.
 */
inline
const View::ContainerType& writes(const View* view)
{
  return view->writes;
}

/**
 * Gets all read accesses contained in a view.
 *
 * @param view A view.
 * @return A list of read accesses contained in the view.
 */
inline
const View::ContainerType& reads(const View* view)
{
  return view->reads;
}

// Type definitions
typedef const View::ContainerType& (*AccessFunctionType)(const View* view);
typedef std::vector< View::ContainerType > Views;

/**
 * Computes an intersection of a view with all views in a window (part of a view
 *   history).
 *
 * @warning The implementation assumes that the window is non-empty!
 *
 * @param view A view which should be intersected with views in a window.
 * @param window A window of views which should be intersected with a view.
 * @return A list containing intersections of a view with all views in a window.
 */
template< AccessFunctionType ViewAccesses, AccessFunctionType HistoryAccesses >
Views intersection(View* view, ViewHistory::Window window)
{
  Views views; // An intersection of a view with all views in a window

  // An iterator pointing to the currently intersected view
  ViewHistory::Iterator it = window.first;

  do
  { // For all view in the window
    View::ContainerType si;

    // Intersect the current view with the specified view
    std::set_intersection(ViewAccesses(view).begin(), ViewAccesses(view).end(),
      HistoryAccesses(*it).begin(), HistoryAccesses(*it).end(),
      std::inserter(si, si.begin()));

    // And store it in the list of intersections
    views.push_back(si);
  } while (it++ != window.last);

  return views; // Return the intersection of a view with all views in a window
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

/**
 * Formats backtraces of a view.
 *
 * @param view A view.
 * @return A string containing backtraces of the view.
 */
inline
std::string backtraces(View* view)
{
  // Helper variables
  std::string output;
  Symbols symbols;

  // Translate the return addresses to locations
  THREAD_GetBacktraceSymbols(view->startbt, symbols);

  output += "  Atomic region start backtrace\n";

  for (Symbols::size_type i = 0; i < symbols.size(); i++)
  { // Print information about each location in the backtrace
    output += "    #" + decstr(i) + (i > 10 ? " " : "  ") + symbols[i] + "\n";
  }

  // Clear the locations before adding new ones
  symbols.clear();

  // Translate the return addresses to locations
  THREAD_GetBacktraceSymbols(view->endbt, symbols);

  output += "  Atomic region end backtrace\n";

  for (Symbols::size_type i = 0; i < symbols.size(); i++)
  { // Print information about each location in the backtrace
    output += "    #" + decstr(i) + (i > 10 ? " " : "  ") + symbols[i] + "\n";
  }

  return output; // Return a string containing the backtraces
}

/**
 * Formats locations accessing memory addresses.
 *
 * @param addresses A set of memory addresses accessed.
 * @param instructions A table mapping memory addresses to a set of locations
 *   accessing them.
 * @return A string containing information about the locations accessing each
 *   of the memory addresses specified.
 */
inline
std::string locations(View::ContainerType addresses,
  std::map< ADDRINT, View::ContainerType > instructions)
{
  // Helper variables
  LOCATION location;
  std::string output;

  for (View::Iterator it = addresses.begin(); it != addresses.end(); it++)
  { // For each memory address accessed, add a list of locations accessing it
    output += "  Locations accessing memory address " + hexstr(*it) + "\n";

    for (View::Iterator iit = instructions[*it].begin();
      iit != instructions[*it].end(); iit++)
    { // Get a source code location corresponding to the obtained instruction
      ACCESS_GetLocation(*iit, location);

      // Append the location to the list of locations accessing the addresses
      output += "    " + location.file + ":" + decstr(location.line) + "\n";
    }
  }

  return output; // Return a string containing information about the locations
}

/**
 * Reports a high-level data race.
 *
 * @param view A view causing a high-level data race.
 * @param window A window causing a high-level data race.
 * @param cvp A pair of views violation a chain.
 */
template< AccessFunctionType ViewAccesses, AccessFunctionType HistoryAccesses >
void report(View* view, ViewHistory::Window window, std::pair< int, int >& cvp)
{
  // Helper variables
  std::string output;

  // As the window is passed by a value, we can modify it safely here
  window.last = window.first;

  // Get the two views violation a chain (and causing a HLDR)
  std::advance(window.first, cvp.first);
  std::advance(window.last, cvp.second);

  // Check if the HLDR is a real one or a possible one
  if ((*window.first)->timestamp > view->timestamp
    && view->timestamp > (*window.last)->timestamp)
  { // We saw the interleaving causing a HLDR, it must be a real one
    output += "Real HLDR!\n";
  }
  else
  { // The interleaving causing a HLDR might not be feasible
    output += "Possible HLDR!\n";
  }

  // Helper variables
  View::ContainerType is;
  std::pair< View::ContainerType, View::ContainerType > cvs;

  // Filter out all accesses not causing the HLDR (not violation the chain)
  std::set_intersection(ViewAccesses(view).begin(), ViewAccesses(view).end(),
    HistoryAccesses(*window.first).begin(), HistoryAccesses(*window.first).end(),
    std::inserter(cvs.first, cvs.first.begin()));
  std::set_intersection(ViewAccesses(view).begin(), ViewAccesses(view).end(),
    HistoryAccesses(*window.last).begin(), HistoryAccesses(*window.last).end(),
    std::inserter(cvs.second, cvs.second.begin()));
  is.insert(cvs.first.begin(), cvs.first.end());
  is.insert(cvs.second.begin(), cvs.second.end());

  CONSOLE(output // Print information about the HLDR
    + decstr((*window.first)->timestamp) + cvs.first
      + decstr((*window.first)->depth) + "\n"
    + backtraces(*window.first)
    + locations(cvs.first, (HistoryAccesses == reads) ? (*window.first)->ris
        : (*window.first)->wis)
    + decstr(view->timestamp) + is + decstr(view->depth) + "\n"
    + backtraces(view)
    + locations(is, (ViewAccesses == reads) ? view->ris : view->wis)
    + decstr((*window.last)->timestamp) + cvs.second
      + decstr((*window.last)->depth) + "\n"
    + backtraces(*window.last)
    + locations(cvs.second, (HistoryAccesses == reads) ? (*window.last)->ris
        : (*window.last)->wis));
}

/**
 * Checks if a view might cause a high-level data race when interleaved with
 *   specific views.
 *
 * @warning The implementation assumes that the window is non-empty!
 *
 * @param view A view that may interleave other views.
 * @param window A window of views which might be interleaved by the specified
 *   view.
 * @return @em True if the view might cause a high-level data race, @em false
 *   otherwise.
 */
bool check(View* view, ViewHistory::Window window)
{
  // Chain violation point, i.e., a pair of views violating the chain
  std::pair< int, int > cvp;

  if (!isChain(intersection< writes, writes >(view, window), cvp))
  { // Check the W/W conflicts
    report< writes, writes >(view, window, cvp);

    return true;
  }
  else if (!isChain(intersection< writes, reads >(view, window), cvp))
  { // Check the W/R conflicts
    report< writes, reads >(view, window, cvp);

    return true;
  }
  else if (!isChain(intersection< reads, writes >(view, window), cvp))
  { // Check the R/W conflicts
    report< reads, writes >(view, window, cvp);

    return true;
  }

  return false; // No high-level data race found
}

/**
 * Checks if a view might cause a high-level data race when interleaved with
 *   the views of other threads.
 *
 * @param tid A number uniquely identifying a thread performing the check.
 * @param view A view.
 * @return @em True if the view might cause a high-level data race, @em false
 *   otherwise.
 */
bool checkThisViewAgainstOtherHistories(THREADID tid, View* view)
{
  // Prevent threads from finishing until we perform the checks
  ScopedReadLock readingRunningThreadsHistories(g_threadsLock);

  for (ThreadIterator it = g_threads.begin(); it != g_threads.end(); it++)
  { // Check the specified view against the histories of other threads
    if (*it != tid)
    { // Acquire a window (part of a view history) of another thread
      LockedWindow window(*it);

      // If the history (window) is empty, no checks are necessary
      if (window.empty()) continue;

      // Check if the view might cause a high-level data race
      if (check(view, window)) return true;
    }
  }

  return false; // No high-level data race found
}

/**
 * Checks if any of the views of other threads might cause a high-level data
 *   race when interleaved with the views of this thread.
 *
 * @param tid A number uniquely identifying a thread performing the check.
 * @return @em True if any of the views of other threads cause a high-level
 *   data race, @em false otherwise.
 */
bool checkOtherViewsAgainstThisHistory(THREADID tid)
{
  // Prevent threads from finishing until we perform the checks
  ScopedReadLock readingRunningThreadsHistories(g_threadsLock);

  // Acquire a window (part of a view history) of this thread
  LockedWindow window(tid);

  for (ThreadIterator it = g_threads.begin(); it != g_threads.end(); it++)
  { // Check the views of other threads against this thread's history
    if (*it != tid)
    { // Acquire a window (part of a view history) of another thread
      LockedWindow views(*it);

      // If the history (window) is empty, there are no views to check
      if (views.empty()) continue;

      // An iterator pointing to the currently checked view
      ViewHistory::Iterator view = views.first();

      do
      { // Check if any of the views might cause a high-level data race
        if (check(*view, window)) return true;
      } while (view++ != views.last());
    }
  }

  return false; // No high-level data race found
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

    THREAD_GetBacktrace(tid, VIEW->startbt); // Save the current backtrace
  }
  else
  {
    VIEW->depth++; // Encountered a nested atomic region
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
 * @param ins An address of the instruction reading from the memory.
 */
inline
VOID memoryRead(THREADID tid, ADDRINT addr, ADDRINT ins)
{
  if (VIEW != NULL)
  { // We are in an atomic region
    VIEW->reads.insert(addr);
    VIEW->ris[addr].insert(ins);
  }
}

/**
 * Updates the current view.
 *
 * @note This function is called when a thread reads from a memory.
 *
 * @param tid A number uniquely identifying a thread writing to the memory.
 * @param addr An address to which the thread written some data.
 * @param ins An address of the instruction writing to the memory.
 */
inline
VOID memoryWritten(THREADID tid, ADDRINT addr, ADDRINT ins)
{
  if (VIEW != NULL)
  { // We are in an atomic region
    VIEW->writes.insert(addr);
    VIEW->wis[addr].insert(ins);
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

  VIEW->timestamp = g_clock++; // Save the time the view was completed

  THREAD_GetBacktrace(tid, VIEW->endbt); // Save the current backtrace

  // First check the current (new) view against the views of other threads
  checkThisViewAgainstOtherHistories(tid, VIEW);

  // Then save the current (new) view to the view history
  VIEW_HISTORY(tid)->insert(VIEW);

  // Finally, check the views of other threads against this thread's views
  checkOtherViewsAgainstThisHistory(tid);

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
  memoryRead(tid, addr, 0);
}

VOID beforeTxWrite(THREADID tid, ADDRINT addr)
{
  memoryWritten(tid, addr, 0);
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
  const VARIABLE& variable, ADDRINT ins, BOOL isLocal)
{
  if (isLocal) return; // Ignore local variables

  memoryRead(tid, addr, ins);
}

VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, ADDRINT ins, BOOL isLocal)
{
  if (isLocal) return; // Ignore local variables

  memoryWritten(tid, addr, ins);
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
