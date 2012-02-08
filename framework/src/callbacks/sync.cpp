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
 * @date      Last Update 2012-02-08
 * @version   0.3
 */

#include "sync.h"

#include <vector>

#include "../settings.h"

// Declarations of static functions (usable only within this module)
static VOID deleteLock(void* lock);
static VOID deleteCond(void* cond);

static VOID afterLockAcquire(THREADID tid);
static VOID afterLockRelease(THREADID tid);
static VOID afterSignal(THREADID tid);
static VOID afterWait(THREADID tid);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_lockTlsKey = PIN_CreateThreadDataKey(deleteLock);
  TLS_KEY g_condTlsKey = PIN_CreateThreadDataKey(deleteCond);

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
 * Deletes a lock object created during thread start.
 *
 * @param lock A lock object.
 */
VOID deleteLock(void* lock)
{
  delete static_cast< LOCK* >(lock);
}

/**
 * Deletes a condition object created during thread start.
 *
 * @param cond A condition object.
 */
VOID deleteCond(void* cond)
{
  delete static_cast< COND* >(cond);
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

  // The created lock must be valid (e.g. the map function cannot return 0)
  assert(lock.is_valid());

  // Return the lock object representing a lock at the specified address
  return lock;
}

/**
 * Gets a lock object representing the last lock accessed by a thread.
 *
 * @param tid A number identifying the thread.
 * @return The lock object representing the last lock accessed by the thread.
 */
inline
LOCK* getLastLock(THREADID tid)
{
  return static_cast< LOCK* >(PIN_GetThreadData(g_lockTlsKey, tid));
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

  // The created condition must be valid (e.g. the map function cannot return 0)
  assert(cond.is_valid());

  // Return the condition object representing a condition at specified address
  return cond;
}

/**
 * Gets a condition object representing the last condition accessed by a thread.
 *
 * @param tid A number identifying the thread.
 * @return The condition object representing the last condition accessed by the
 *   thread.
 */
inline
COND* getLastCondition(THREADID tid)
{
  return static_cast< COND* >(PIN_GetThreadData(g_condTlsKey, tid));
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
 * Concatenates a string with a lock object.
 *
 * @param s A string.
 * @param lock A lock object.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em lock.
 */
std::string operator+(const std::string& s, const LOCK& lock)
{
  return s + "LOCK(index=" + decstr(lock.q()) + ")";
}

/**
 * Concatenates a lock object with a string.
 *
 * @param lock A lock object.
 * @param s A string
 * @return A new string with a value of a string representation of @em lock
 *   followed by @em s.
 */
std::string operator+(const LOCK& lock, const std::string& s)
{
  return "LOCK(index=" + decstr(lock.q()) + ")" + s;
}

/**
 * Concatenates a string with a condition object.
 *
 * @param s A string.
 * @param cond A condition object.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em cond.
 */
std::string operator+(const std::string& s, const COND& cond)
{
  return s + "COND(index=" + decstr(cond.q()) + ")";
}

/**
 * Concatenates a condition object with a string.
 *
 * @param cond A condition object.
 * @param s A string.
 * @return A new string with a value of a string representation of @em cond
 *   followed by @em s.
 */
std::string operator+(const COND& cond, const std::string& s)
{
  return "COND(index=" + decstr(cond.q()) + ")" + s;
}

/**
 * Initialises TLS (thread local storage) data for a thread.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID initSyncFunctionTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // Each thread has a lock and a condition object associated with it
  LOCK* lock = new LOCK();
  COND* cond = new COND();

  // Invalidate the objects (this is the initial state for assertion checking)
  lock->invalidate();
  cond->invalidate();

  // Set the objects once, after it, they will be overwritten when needed
  PIN_SetThreadData(g_lockTlsKey, lock, tid);
  PIN_SetThreadData(g_condTlsKey, cond, tid);
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockAcquire(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, VOID* funcDesc)
{
  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a lock function in the same thread again before leaving it
  assert(!getLastLock(tid)->is_valid());

  // Save the accessed lock for the time when the lock function if left
  *getLastLock(tid) = lock;

  for (LockFunPtrVector::iterator it = g_beforeLockAcquireVector.begin();
    it != g_beforeLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }

  // Register a callback function to be called after acquiring the lock
  CALL_AFTER(afterLockAcquire);
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockRelease(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, VOID* funcDesc)
{
  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter an unlock function in the same thread again before leaving it
  assert(!getLastLock(tid)->is_valid());

  // Save the accessed lock for the time when the unlock function if left
  *getLastLock(tid) = lock;

  for (LockFunPtrVector::iterator it = g_beforeLockReleaseVector.begin();
    it != g_beforeLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }

  // Register a callback function to be called after releasing the lock
  CALL_AFTER(afterLockRelease);
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the condition stored.
 * @param funcDesc A structure containing the description of the function which
 *   signalled the condition.
 */
VOID beforeSignal(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, VOID* funcDesc)
{
  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a signal function in the same thread again before leaving it
  assert(!getLastCondition(tid)->is_valid());

  // Save the accessed condition for the time when the signal function if left
  *getLastCondition(tid) = cond;

  for (CondFunPtrVector::iterator it = g_beforeSignalVector.begin();
    it != g_beforeSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }

  // Register a callback function to be called after sending a signal
  CALL_AFTER(afterSignal);
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the condition stored.
 * @param funcDesc A structure containing the description of the function which
 *   is waiting on the condition.
 */
VOID beforeWait(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, VOID* funcDesc)
{
  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a wait function in the same thread again before leaving it
  assert(!getLastCondition(tid)->is_valid());

  // Save the accessed condition for the time when the wait function if left
  *getLastCondition(tid) = cond;

  for (CondFunPtrVector::iterator it = g_beforeWaitVector.begin();
    it != g_beforeWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }

  // Register a callback function to be called after waiting
  CALL_AFTER(afterWait);
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 */
VOID afterLockAcquire(THREADID tid)
{
  // The lock acquired must be the last one accessed by the thread
  LOCK* lock = getLastLock(tid);

  // Cannot leave a lock function before entering it
  assert(lock->is_valid());

  for (LockFunPtrVector::iterator it = g_afterLockAcquireVector.begin();
    it != g_afterLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, *lock);
  }

  // This will tell the asserts that we left the lock function
  lock->invalidate();
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 */
VOID afterLockRelease(THREADID tid)
{
  // The lock acquired must be the last one accessed by the thread
  LOCK* lock = getLastLock(tid);

  // Cannot leave an unlock function before entering it
  assert(lock->is_valid());

  for (LockFunPtrVector::iterator it = g_afterLockReleaseVector.begin();
    it != g_afterLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, *lock);
  }

  // This will tell the asserts that we left the unlock function
  lock->invalidate();
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 */
VOID afterSignal(THREADID tid)
{
  // The condition accessed must be the last one accessed by the thread
  COND* cond = getLastCondition(tid);

  // Cannot leave a signal function before entering it
  assert(cond->is_valid());

  for (CondFunPtrVector::iterator it = g_afterSignalVector.begin();
    it != g_afterSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, *cond);
  }

  // This will tell the asserts that we left the signal function
  cond->invalidate();
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 */
VOID afterWait(THREADID tid)
{
  // The condition accessed must be the last one accessed by the thread
  COND* cond = getLastCondition(tid);

  // Cannot leave a wait function before entering it
  assert(cond->is_valid());

  for (CondFunPtrVector::iterator it = g_afterWaitVector.begin();
    it != g_afterWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, *cond);
  }

  // This will tell the asserts that we left the wait function
  cond->invalidate();
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
