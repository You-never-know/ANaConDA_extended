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
 * @date      Last Update 2013-07-25
 * @version   0.9.2
 */

#include "sync.h"

#include <vector>

#include "shared.hpp"

#include "../anaconda.h"
#include "../settings.h"

#include "../monitors/sync.hpp"

#include "../utils/rwmap.hpp"
#include "../utils/tldata.hpp"

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
static VOID afterLockCreate(THREADID tid, ADDRINT* retVal, VOID* data);
template< ConcurrentCoverage CC >
static VOID afterLockAcquire(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterLockRelease(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterSignal(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterWait(THREADID tid, ADDRINT* retVal, VOID* data);

/**
 * @brief A structure holding private data of a thread.
 */
typedef struct ThreadData_s
{
  LOCK lock; //!< The last lock accessed by a thread.
  COND cond; //!< The last condition accessed by a thread.

  /**
   * Constructs a ThreadData_s object.
   */
  ThreadData_s() : lock(), cond()
  {
    // Do not assume that the default constructor will invalidate the object
    lock.invalidate();
    cond.invalidate();
  }
} ThreadData;

namespace
{ // Static global variables (usable only within this module)
  ThreadLocalData< ThreadData > g_data; //!< Private data of running threads.

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
 * @param arg A pointer to the argument representing the lock which is about
 *   to be acquired.
 * @param hi A structure containing information about a function working with
 *   the lock.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeLockAcquire(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  // Register a callback function to be called after acquiring the lock
  if (CALL_AFTER(afterLockAcquire< CC >)) return;

  // Get the lock stored at the specified address
  LOCK lock = mapArgTo< LOCK >(arg, hi);

  // Cannot enter a lock function in the same thread again before leaving it
  assert(!g_data.get(tid)->lock.is_valid());

  // Save the accessed lock for the time when the lock function if left
  g_data.get(tid)->lock = lock;

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
 * @param arg A pointer to the argument representing the lock which is about
 *   to be released.
 * @param hi A structure containing information about a function working with
 *   the lock.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeLockRelease(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  // Register a callback function to be called after releasing the lock
  if (CALL_AFTER(afterLockRelease)) return;

  // Get the lock stored at the specified address
  LOCK lock = mapArgTo< LOCK >(arg, hi);

  // Cannot enter an unlock function in the same thread again before leaving it
  assert(!g_data.get(tid)->lock.is_valid());

  // Save the accessed lock for the time when the unlock function if left
  g_data.get(tid)->lock = lock;

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
 * @param arg A pointer to the argument representing the condition which is
 *   about to be signalled.
 * @param hi A structure containing information about a function working with
 *   the condition.
 */
VOID beforeSignal(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  // Register a callback function to be called after sending a signal
  if (CALL_AFTER(afterSignal)) return;

  // Get the condition stored at the specified address
  COND cond = mapArgTo< COND >(arg, hi);

  // Cannot enter a signal function in the same thread again before leaving it
  assert(!g_data.get(tid)->cond.is_valid());

  // Save the accessed condition for the time when the signal function if left
  g_data.get(tid)->cond = cond;

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
 * @param arg A pointer to the argument representing the condition which is
 *   the thread about to start waiting for.
 * @param hi A structure containing information about a function working with
 *   the condition.
 */
VOID beforeWait(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  // Register a callback function to be called after waiting
  if (CALL_AFTER(afterWait)) return;

  // Get the condition stored at the specified address
  COND cond = mapArgTo< COND >(arg, hi);

  // Cannot enter a wait function in the same thread again before leaving it
  assert(!g_data.get(tid)->cond.is_valid());

  // Save the accessed condition for the time when the wait function if left
  g_data.get(tid)->cond = cond;

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
 * @param arg A pointer to the argument representing an arbitrary object which
 *   is the thread about to start waiting for.
 * @param hi A structure containing information about a function working with
 *   the object.
 */
template< ConcurrentCoverage CC >
inline
VOID beforeGenericWait(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  switch (getObjectType(arg, hi))
  { // Trigger appropriate notifications based on the type of the object
    case OBJ_UNKNOWN: // An unknown object, ignore it
      break;
    case OBJ_LOCK: // A lock, trigger lock acquisition notifications
      beforeLockAcquire< CC >(tid, sp, arg, hi);
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
  LOCK lock = mapArgTo< LOCK >(retVal, static_cast< HookInfo* >(data));

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
  LOCK& lock = g_data.get(tid)->lock;

  // Cannot leave a lock function before entering it
  assert(lock.is_valid());

  for (LockFunPtrVector::iterator it = g_afterLockAcquireVector.begin();
    it != g_afterLockAcquireVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }

  // This will tell the asserts that we left the lock function
  lock.invalidate();
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 */
VOID afterLockRelease(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // The lock acquired must be the last one accessed by the thread
  LOCK& lock = g_data.get(tid)->lock;

  // Cannot leave an unlock function before entering it
  assert(lock.is_valid());

  for (LockFunPtrVector::iterator it = g_afterLockReleaseVector.begin();
    it != g_afterLockReleaseVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, lock);
  }

  // This will tell the asserts that we left the unlock function
  lock.invalidate();
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 */
VOID afterSignal(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // The condition accessed must be the last one accessed by the thread
  COND& cond = g_data.get(tid)->cond;

  // Cannot leave a signal function before entering it
  assert(cond.is_valid());

  for (CondFunPtrVector::iterator it = g_afterSignalVector.begin();
    it != g_afterSignalVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }

  // This will tell the asserts that we left the signal function
  cond.invalidate();
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 */
VOID afterWait(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // The condition accessed must be the last one accessed by the thread
  COND& cond = g_data.get(tid)->cond;

  // Cannot leave a wait function before entering it
  assert(cond.is_valid());

  for (CondFunPtrVector::iterator it = g_afterWaitVector.begin();
    it != g_afterWaitVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, cond);
  }

  // This will tell the asserts that we left the wait function
  cond.invalidate();
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
 * Setups the synchronisation function monitoring, i.e., setups the functions
 *   which will be used for instrumenting the synchronisation functions etc.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupSyncModule(Settings* settings)
{
  g_syncCovMon = &settings->getCoverageMonitors().sync;
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
