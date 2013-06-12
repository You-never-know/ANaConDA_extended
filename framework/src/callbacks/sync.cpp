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
 * @date      Last Update 2013-06-12
 * @version   0.5
 */

#include "sync.h"

#include <vector>

#include "thread.h"

#include "../settings.h"

#include "../monitors/sync.hpp"

#include "../utils/rwmap.hpp"

// Helper macros
#define CALL_AFTER(callback) \
  REGISTER_AFTER_CALLBACK(callback, static_cast< VOID* >(hi))

/**
 * @brief An enumeration of objects on which may a generic wait function wait.
 */
typedef enum ObjectType_e
{
  OBJ_UNKNOWN, //!< An unknown object.
  OBJ_LOCK     //!< A lock.
} ObjectType;

// Declarations of static functions (usable only within this module)
static VOID deleteLock(void* lock);
static VOID deleteCond(void* cond);

static VOID afterLockCreate(THREADID tid, ADDRINT* retVal, VOID* data);
template< ConcurrentCoverage CC >
static VOID afterLockAcquire(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterLockRelease(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterSignal(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterWait(THREADID tid, ADDRINT* retVal, VOID* data);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_lockTlsKey = PIN_CreateThreadDataKey(deleteLock);
  TLS_KEY g_condTlsKey = PIN_CreateThreadDataKey(deleteCond);

  /**
   * @brief A concurrent map containing objects on which a generic wait function
   *   is waiting.
   */
  RWMap< UINT32, ObjectType > g_objectTypeMap(OBJ_UNKNOWN);

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

  SyncCoverageMonitor< FileWriter >* g_syncCovMon;
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
 * @param hi A structure containing information about a function working with
 *   the lock at the specified address.
 * @return The lock object representing the lock at the specified address.
 */
inline
LOCK getLock(ADDRINT* lockAddr, HookInfo* hi)
{
  for (int lvl = hi->refdepth; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the lock, get to it
    lockAddr = reinterpret_cast< ADDRINT* >(*lockAddr);
  }

  // Lock objects must be created in two steps, first create a lock object
  LOCK lock;
  // Then modify it to create a lock object for the specified address
  lock.q_set(hi->mapper->map(lockAddr));

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
 * @param hi A structure containing information about a function working with
 *   the condition at the specified address.
 * @return The lock object representing the condition at the specified address.
 */
inline
COND getCondition(ADDRINT* condAddr, HookInfo* hi)
{
  for (int lvl = hi->refdepth; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the condition, get to it
    condAddr = reinterpret_cast< ADDRINT* >(*condAddr);
  }

  // Condition objects must be created in two steps, first create the object
  COND cond;
  // Then modify it to create a condition object for the specified address
  cond.q_set(hi->mapper->map(condAddr));

  // The created condition must be valid (e.g. the map function cannot return 0)
  assert(cond.is_valid());

  // Return the condition object representing a condition at specified address
  return cond;
}

/**
 * Gets a type of the object at a specific address.
 *
 * @param wobjAddr An address at which is the object stored.
 * @param hi A structure containing information about a function working with
 *    the object at the specified address.
 * @return The type of the object at the specified address.
 */
inline
ObjectType getObjectType(ADDRINT* wobjAddr, HookInfo* hi)
{
  for (int lvl = hi->refdepth; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the condition, get to it
    wobjAddr = reinterpret_cast< ADDRINT* >(*wobjAddr);
  }

  // Return the type of the object on which is a generic wait function waiting
  return g_objectTypeMap.get(hi->mapper->map(wobjAddr));
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
 * Setups the synchronisation coverage monitoring.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupSyncModule(Settings* settings)
{
  g_syncCovMon = &settings->getCoverageMonitors().sync;
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
 * Registers a callback function which will be called after a function creates
 *   a lock and store information about the lock.
 *
 * @param tid A thread which is about to create a lock.
 * @param sp A value of the stack pointer register.
 * @param hi A structure containing information about a function creating the
 *   lock.
 */
VOID beforeLockCreate(CBSTACK_FUNC_PARAMS, HookInfo* hi)
{
  // Register a callback function to be called after creating the lock
  if (CALL_AFTER(afterLockCreate)) return;
}

/**
 * Notifies all listeners that a thread is about to acquire a lock.
 *
 * @param tid A thread which is about to acquire a lock.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the lock stored.
 * @param hi A structure containing information about a function working with
 *   the lock.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeLockAcquire(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, HookInfo* hi)
{
  // Register a callback function to be called after acquiring the lock
  if (CALL_AFTER(afterLockAcquire< CC >)) return;

  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, hi);

  // Cannot enter a lock function in the same thread again before leaving it
  assert(!getLastLock(tid)->is_valid());

  // Save the accessed lock for the time when the lock function if left
  *getLastLock(tid) = lock;

  if (CC & CC_SYNC)
  { // Notify the sync coverage monitor that we are about acquire a lock
    g_syncCovMon->beforeLockAcquired(lock, getLastBacktraceLocationIndex(tid));
  }

  for (LockFunPtrVector::iterator it = g_beforeLockAcquireVector.begin();
    it != g_beforeLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }
}

/**
 * Notifies all listeners that a thread is about to release a lock.
 *
 * @param tid A thread which is about to release a lock.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the lock stored.
 * @param hi A structure containing information about a function working with
 *   the lock.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeLockRelease(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, HookInfo* hi)
{
  // Register a callback function to be called after releasing the lock
  if (CALL_AFTER(afterLockRelease)) return;

  // Get the lock stored at the specified address
  LOCK lock = getLock(lockAddr, hi);

  // Cannot enter an unlock function in the same thread again before leaving it
  assert(!getLastLock(tid)->is_valid());

  // Save the accessed lock for the time when the unlock function if left
  *getLastLock(tid) = lock;

  if (CC & CC_SYNC)
  { // Notify the sync coverage monitor that we are about release a lock
    g_syncCovMon->beforeLockReleased(lock, getLastBacktraceLocationIndex(tid));
  }

  for (LockFunPtrVector::iterator it = g_beforeLockReleaseVector.begin();
    it != g_beforeLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }
}

/**
 * Notifies all listeners that a thread is about to signal a condition.
 *
 * @param tid A thread which is about to signal a condition.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the condition stored.
 * @param hi A structure containing information about a function working with
 *   the condition.
 */
VOID beforeSignal(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, HookInfo* hi)
{
  // Register a callback function to be called after sending a signal
  if (CALL_AFTER(afterSignal)) return;

  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, hi);

  // Cannot enter a signal function in the same thread again before leaving it
  assert(!getLastCondition(tid)->is_valid());

  // Save the accessed condition for the time when the signal function if left
  *getLastCondition(tid) = cond;

  for (CondFunPtrVector::iterator it = g_beforeSignalVector.begin();
    it != g_beforeSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }
}

/**
 * Notifies all listeners that a thread is about to wait for a condition.
 *
 * @param tid A thread which is about to wait for a condition.
 * @param sp A value of the stack pointer register.
 * @param lockAddr An address at which is the condition stored.
 * @param hi A structure containing information about a function working with
 *   the condition.
 */
VOID beforeWait(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, HookInfo* hi)
{
  // Register a callback function to be called after waiting
  if (CALL_AFTER(afterWait)) return;

  // Get the condition stored at the specified address
  COND cond = getCondition(condAddr, hi);

  // Cannot enter a wait function in the same thread again before leaving it
  assert(!getLastCondition(tid)->is_valid());

  // Save the accessed condition for the time when the wait function if left
  *getLastCondition(tid) = cond;

  for (CondFunPtrVector::iterator it = g_beforeWaitVector.begin();
    it != g_beforeWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }
}

/**
 * Triggers appropriate notifications based on the type of the object for which
 *   is a thread about to wait.
 *
 * @param tid A thread which is about to wait for an object.
 * @param sp A value of the stack pointer register.
 * @param wobjAddr An address at which is the object stored.
 * @param hi A structure containing information about a function working with
 *   the object.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeGenericWait(CBSTACK_FUNC_PARAMS, ADDRINT* wobjAddr, HookInfo* hi)
{
  switch (getObjectType(wobjAddr, hi))
  { // Trigger appropriate notifications based on the type of the object
    case OBJ_UNKNOWN: // An unknown object, ignore it
      break;
    case OBJ_LOCK: // A lock, trigger lock acquisition notifications
      beforeLockAcquire< CC >(tid, sp, wobjAddr, hi);
      break;
    default: // Something is very wrong if the control reaches here
      assert(false);
      break;
  }
}

/**
 * Stores information about a created lock.
 *
 * @param tid A thread which executed the function which created a lock.
 * @param retVal A return value of the function which created a lock.
 * @param data Arbitrary data associated with the call to this function.
 */
VOID afterLockCreate(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // Get the lock created by the function which execution just finished
  LOCK lock = getLock(retVal, static_cast< HookInfo* >(data));

  // Remember that the synchronisation primitive is a lock
  g_objectTypeMap.insert(lock.q(), OBJ_LOCK);
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 */
template< ConcurrentCoverage CC >
inline
VOID afterLockAcquire(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // The lock acquired must be the last one accessed by the thread
  LOCK* lock = getLastLock(tid);

  // Cannot leave a lock function before entering it
  assert(lock->is_valid());

  if (CC & CC_SYNC)
  { // Notify the sync coverage monitor that we just acquired a lock
    g_syncCovMon->afterLockAcquired(*lock, getLastBacktraceLocationIndex(tid));
  }

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
VOID afterLockRelease(THREADID tid, ADDRINT* retVal, VOID* data)
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
VOID afterSignal(THREADID tid, ADDRINT* retVal, VOID* data)
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
VOID afterWait(THREADID tid, ADDRINT* retVal, VOID* data)
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
 * @brief Instantiates a concrete code of callback functions from its templates.
 *
 * @param coverage A type of concurrent coverage which should be monitored.
 */
#define INSTANTIATE_CALLBACK_FUNCTIONS(coverage) \
  template VOID PIN_FAST_ANALYSIS_CALL beforeLockAcquire< coverage > \
    (CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, HookInfo* hi); \
  template VOID PIN_FAST_ANALYSIS_CALL beforeLockRelease< coverage > \
    (CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, HookInfo* hi); \
  template VOID PIN_FAST_ANALYSIS_CALL beforeGenericWait< coverage > \
    (CBSTACK_FUNC_PARAMS, ADDRINT* wobjAddr, HookInfo* hi); \
  template VOID PIN_FAST_ANALYSIS_CALL afterLockAcquire< coverage > \
    (THREADID tid, ADDRINT* retVal, VOID* data)

// Instantiate callback functions for specific concurrent coverage types
INSTANTIATE_CALLBACK_FUNCTIONS(CC_NONE);
INSTANTIATE_CALLBACK_FUNCTIONS(CC_SYNC);

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
