/**
 * @brief Contains the entry part of the event printer ANaConDA plugin.
 *
 * A file containing the entry part of the event printer ANaConDA plugin.
 *
 * @file      event-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-05
 * @date      Last Update 2012-01-15
 * @version   0.1.1
 */

#include <iostream>

#include "anaconda.h"

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  std::cout << "Before lock acquired: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  std::cout << "Before lock released: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID beforeSignal(THREADID tid, COND cond)
{
  std::cout << "Before signal send: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param cond An object representing the condition on which is the thread
 *   waiting.
 */
VOID beforeWait(THREADID tid, COND cond)
{
  std::cout << "Before wait: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  std::cout << "After lock acquired: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID afterLockRelease(THREADID tid, LOCK lock)
{
  std::cout << "After lock released: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID afterSignal(THREADID tid, COND cond)
{
  std::cout << "After signal send: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param cond An object representing the condition on which is the thread
 *   waiting.
 */
VOID afterWait(THREADID tid, COND cond)
{
  std::cout << "After wait: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

/**
 * Initialises the event printer plugin.
 */
extern "C"
void init()
{
  // Register callback functions called before synchronisation events
  SYNC_BeforeLockAcquire(beforeLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);
  SYNC_BeforeSignal(beforeSignal);
  SYNC_BeforeWait(beforeWait);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_AfterLockRelease(afterLockRelease);
  SYNC_AfterSignal(afterSignal);
  SYNC_AfterWait(afterWait);
}

/** End of file event-printer.cpp **/
