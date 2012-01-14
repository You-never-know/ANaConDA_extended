/**
 * @brief A file containing implementation of synchronisation-related callback
 *   functions.
 *
 * A file containing implementation of callback functions called when some
 *   synchronisation between threads occurs.
 *
 * @file      sync.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-01-14
 * @version   0.1.5
 */

#include "sync.h"

#include <iostream>
#include <map>

#include "../settings.h"

namespace
{ // Static global variables (usable only within this module)
  std::map< THREADID, LOCK > g_lockMap;
  std::map< THREADID, COND > g_condMap;

  typedef std::vector< LOCKFUNPTR > LockFunPtrVector;
  typedef std::vector< CONDFUNPTR > CondFunPtrVector;

  LockFunPtrVector g_beforeLockAcquireVector;
  LockFunPtrVector g_beforeLockReleaseVector;
  CondFunPtrVector g_beforeSignalVector;
  CondFunPtrVector g_beforeWaitVector;

  LockFunPtrVector g_afterLockAcquireVector;
  LockFunPtrVector g_afterLockReleaseVector;
  CondFunPtrVector g_afterSignalVector;
  CondFunPtrVector g_afterWaitVector;
}

/**
 * Gets a lock object representing a lock at a specific address.
 *
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock at the specified address.
 * @return The lock object representing the lock at the specified address.
 */
inline
LOCK getLock(ADDRINT* lockAddr, FunctionDesc* funcDesc)
{
  for (int lvl = funcDesc->plvl; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the lock, get to it
    lockAddr = reinterpret_cast< ADDRINT* >(*lockAddr);
  }

  // Lock objects must be created in two steps, first create a lock object
  LOCK lock;
  // Then modify it to create a lock object for the specified address
  lock.q_set(funcDesc->farg->map(lockAddr));

  // Return the lock object representing a lock at the specified address
  return lock;
}

/**
 * Gets a condition object representing a condition at a specific address.
 *
 * @param lockAddr An address at which is the condition stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the condition at the specified address.
 * @return The lock object representing the condition at the specified address.
 */
inline
COND getCondition(ADDRINT* condAddr, FunctionDesc* funcDesc)
{
  for (int lvl = funcDesc->plvl; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the condition, get to it
    condAddr = reinterpret_cast< ADDRINT* >(*condAddr);
  }

  // Condition objects must be created in two steps, first create the object
  COND cond;
  // Then modify it to create a condition object for the specified address
  cond.q_set(funcDesc->farg->map(condAddr));

  // Return the condition object representing a condition at specified address
  return cond;
}

/**
 * Prints a lock object to a stream.
 *
 * @param s A stream to which the lock object should be printed.
 * @param value A lock object.
 * @return The stream to which was the lock object printed.
 */
std::ostream& operator<<(std::ostream& s, const LOCK& value)
{
  // Print the lock object to the stream
  s << "LOCK(index=" << value.q() << ")";

  // Return the stream to which was the lock object printed
  return s;
}

/**
 * Prints a condition object to a stream.
 *
 * @param s A stream to which the condition object should be printed.
 * @param value A condition object.
 * @return The stream to which was the condition object printed.
 */
std::ostream& operator<<(std::ostream& s, const COND& value)
{
  // Print the condition object to the stream
  s << "COND(index=" << value.q() << ")";

  // Return the stream to which was the condition object printed
  return s;
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockAcquire(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc)
{
  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a lock function in the same thread again before leaving it
  assert(g_lockMap.find(tid) == g_lockMap.end());

  // Save the accessed lock for the time when the lock function if left
  g_lockMap[tid] = lock;

  for (LockFunPtrVector::iterator it = g_beforeLockAcquireVector.begin();
    it != g_beforeLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockRelease(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc)
{
  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter an unlock function in the same thread again before leaving it
  assert(g_lockMap.find(tid) == g_lockMap.end());

  // Save the accessed lock for the time when the unlock function if left
  g_lockMap[tid] = lock;

  for (LockFunPtrVector::iterator it = g_beforeLockReleaseVector.begin();
    it != g_beforeLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param lockAddr An address at which is the condition stored.
 * @param funcDesc A structure containing the description of the function which
 *   signalled the condition.
 */
VOID beforeSignal(THREADID tid, ADDRINT* condAddr, VOID* funcDesc)
{
  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a signal function in the same thread again before leaving it
  assert(g_condMap.find(tid) == g_condMap.end());

  // Save the accessed condition for the time when the signal function if left
  g_condMap[tid] = cond;

  for (CondFunPtrVector::iterator it = g_beforeSignalVector.begin();
    it != g_beforeSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param lockAddr An address at which is the condition stored.
 * @param funcDesc A structure containing the description of the function which
 *   is waiting on the condition.
 */
VOID beforeWait(THREADID tid, ADDRINT* condAddr, VOID* funcDesc)
{
  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a wait function in the same thread again before leaving it
  assert(g_condMap.find(tid) == g_condMap.end());

  // Save the accessed condition for the time when the wait function if left
  g_condMap[tid] = cond;

  for (CondFunPtrVector::iterator it = g_beforeWaitVector.begin();
    it != g_beforeWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 */
VOID afterLockAcquire(THREADID tid)
{
  // Cannot leave a lock function before entering it
  assert(g_lockMap.find(tid) != g_lockMap.end());

  for (LockFunPtrVector::iterator it = g_afterLockAcquireVector.begin();
    it != g_afterLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_lockMap[tid]);
  }

  // No need to keep the acquired lock saved anymore
  g_lockMap.erase(tid);
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 */
VOID afterLockRelease(THREADID tid)
{
  // Cannot leave an unlock function before entering it
  assert(g_lockMap.find(tid) != g_lockMap.end());

  for (LockFunPtrVector::iterator it = g_afterLockReleaseVector.begin();
    it != g_afterLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_lockMap[tid]);
  }

  // No need to keep the released lock saved anymore
  g_lockMap.erase(tid);
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 */
VOID afterSignal(THREADID tid)
{
  // Cannot leave a signal function before entering it
  assert(g_condMap.find(tid) != g_condMap.end());

  for (CondFunPtrVector::iterator it = g_afterSignalVector.begin();
    it != g_afterSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_condMap[tid]);
  }

  // No need to keep the condition saved anymore
  g_condMap.erase(tid);
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 */
VOID afterWait(THREADID tid)
{
  // Cannot leave a wait function before entering it
  assert(g_condMap.find(tid) != g_condMap.end());

  for (CondFunPtrVector::iterator it = g_afterWaitVector.begin();
    it != g_afterWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_condMap[tid]);
  }

  // No need to keep the condition saved anymore
  g_condMap.erase(tid);
}

/**
 * Registers a callback function which will be called before acquiring a lock.
 *
 * @param callback A callback function which should be called before acquiring
 *   a lock.
 */
VOID SYNC_BeforeLockAcquire(LOCKFUNPTR callback)
{
  g_beforeLockAcquireVector.push_back(callback);
}

/**
 * Registers a callback function which will be called before releasing a lock.
 *
 * @param callback A callback function which should be called before releasing
 *   a lock.
 */
VOID SYNC_BeforeLockRelease(LOCKFUNPTR callback)
{
  g_beforeLockReleaseVector.push_back(callback);
}

/**
 * Registers a callback function which will be called before sending a signal.
 *
 * @param callback A callback function which should be called before sending
 *   a signal.
 */
VOID SYNC_BeforeSignal(CONDFUNPTR callback)
{
  g_beforeSignalVector.push_back(callback);
}

/**
 * Registers a callback function which will be called before waiting for
 *   a signal.
 *
 * @param callback A callback function which should be called before waiting
 *   for a signal.
 */
VOID SYNC_BeforeWait(CONDFUNPTR callback)
{
  g_beforeWaitVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after acquiring a lock.
 *
 * @param callback A callback function which should be called after acquiring
 *   a lock.
 */
VOID SYNC_AfterLockAcquire(LOCKFUNPTR callback)
{
  g_afterLockAcquireVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after releasing a lock.
 *
 * @param callback A callback function which should be called after releasing
 *   a lock.
 */
VOID SYNC_AfterLockRelease(LOCKFUNPTR callback)
{
  g_afterLockReleaseVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after sending a signal.
 *
 * @param callback A callback function which should be called after sending
 *   a signal.
 */
VOID SYNC_AfterSignal(CONDFUNPTR callback)
{
  g_afterSignalVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after waiting for
 *   a signal.
 *
 * @param callback A callback function which should be called after waiting
 *   for a signal.
 */
VOID SYNC_AfterWait(CONDFUNPTR callback)
{
  g_afterWaitVector.push_back(callback);
}

/** End of file sync.cpp **/
