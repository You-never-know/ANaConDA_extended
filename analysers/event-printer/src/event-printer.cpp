/**
 * @brief Contains the entry part of the event printer ANaConDA plugin.
 *
 * A file containing the entry part of the event printer ANaConDA plugin.
 *
 * @file      event-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-05
 * @date      Last Update 2012-02-03
 * @version   0.1.4
 */

#include "anaconda.h"

/**
 * Gets a declaration of a variable.
 *
 * @param variable A structure containing the information about the variable.
 * @return A string containing the declaration of the variable.
 */
inline
std::string getVariableDeclaration(const VARIABLE& variable)
{
  // Format the name, type and offset to a 'type name[+offset]' string
  return ((variable.type.size() == 0) ? "" : variable.type + " ")
    + ((variable.name.empty()) ? "<unknown>" : variable.name)
    + ((variable.offset == 0) ? "" : "+" + decstr(variable.offset));
}

/**
 * Prints information about a read from a memory.
 *
 * @param tid A thread which performed the read.
 * @param addr An address from which were the data read.
 * @param size A size in bytes of the data read.
 * @param variable A structure containing information about a variable stored
 *   at the address from which were the data read.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  CONSOLE("Thread " + decstr(tid)
    + " read " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " from " + getVariableDeclaration(variable)
    + " [address " + hexstr(addr) + "]\n");
}

/**
 * Prints information about a write to a memory.
 *
 * @param tid A thread which performed the write.
 * @param addr An address to which were the data written.
 * @param size A size in bytes of the data written.
 * @param variable A structure containing information about a variable stored
 *   at the address to which were the data written.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  CONSOLE("Thread " + decstr(tid)
    + " written " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " to " + getVariableDeclaration(variable)
    + " [address " + hexstr(addr) + "]\n");
}

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
 * Prints information about a thread which is about to start.
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  CONSOLE("Thread " + decstr(tid) + " started.");
}

/**
 * Prints information about a thread which is about to finish.
 *
 * @param tid A number identifying the thread.
 */
VOID threadFinished(THREADID tid)
{
  CONSOLE("Thread " + decstr(tid) + " finished.");
}

/**
 * Initialises the event printer plugin.
 */
extern "C"
void init()
{
  // Register callback functions called before access events
  ACCESS_BeforeMemoryRead(beforeMemoryRead);
  ACCESS_BeforeMemoryWrite(beforeMemoryWrite);

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

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);
}

/** End of file event-printer.cpp **/
