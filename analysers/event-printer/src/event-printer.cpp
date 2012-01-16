/**
 * @brief Contains the entry part of the event printer ANaConDA plugin.
 *
 * A file containing the entry part of the event printer ANaConDA plugin.
 *
 * @file      event-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-05
 * @date      Last Update 2012-01-16
 * @version   0.1.1.1
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
  CONSOLE("Before lock acquired: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  CONSOLE("Before lock released: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID beforeSignal(THREADID tid, COND cond)
{
  CONSOLE("Before signal send: thread " + decstr(tid) + ", condition " + cond
    + "\n");
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
  CONSOLE("Before wait: thread " + decstr(tid) + ", condition " + cond + "\n");
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  CONSOLE("After lock acquired: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID afterLockRelease(THREADID tid, LOCK lock)
{
  CONSOLE("After lock released: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID afterSignal(THREADID tid, COND cond)
{
  CONSOLE("After signal send: thread " + decstr(tid) + ", condition " + cond
    + "\n");
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
  CONSOLE("After wait: thread " + decstr(tid) + ", condition " + cond + "\n");
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
