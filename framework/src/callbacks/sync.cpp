/**
 * @brief Contains implementation of functions for monitoring synchronisation
 *   operations.
 *
 * A file containing implementation of functions for monitoring synchronisation
 *   operations.
 *
 * @file      sync.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-08-02
 * @version   0.10.2
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

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief An enumeration describing the types of synchronisation operations.
   */
  typedef enum SyncOperationType_e
  {
    ACQUIRE, //!< A lock acquired operation.
    RELEASE, //!< A lock released operation.
    SIGNAL,  //!< A condition signalled operation.
    WAIT,    //!< A wait for condition operation.
    JOIN     //!< A two threads joined operation.
  } SyncOperationType;

  /**
   * @brief An enumeration of objects for which may a generic wait function wait.
   */
  typedef enum ObjectType_e
  {
    OT_UNKNOWN, //!< An unknown object.
    OT_LOCK,    //!< A lock.
    OT_THREAD   //!< A thread.
  } ObjectType;

  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    LOCK lock; //!< The last lock accessed by a thread.
    COND cond; //!< The last condition accessed by a thread.
    THREAD thread; //!< The last thread joined with a thread.

    /**
     * Constructs a ThreadData_s object.
     */
    ThreadData_s() : lock(), cond(), thread()
    {
      // Do not assume that the default constructor will invalidate the object
      lock.invalidate();
      cond.invalidate();
      thread.invalidate();
    }
  } ThreadData;

  ThreadLocalData< ThreadData > g_data; //!< Private data of running threads.

  /**
   * @brief A concurrent map containing objects for which a generic wait
   *   function is waiting.
   */
  RWMap< UINT32, ObjectType > g_objectTypeMap(OT_UNKNOWN);
}

/**
 * @brief A structure containing sync traits information.
 */
template < SyncOperationType OT >
struct SyncTraits
{
};

/**
 * @brief Defines sync traits information for a specific type of operations.
 *
 * @param optype A type of the operation (constants from the SyncOperationType
 *   enumeration).
 * @param sptype A type of the synchronisation primitive used by the operation
 *   (e.g., LOCK, COND or THREAD structures).
 * @param spfield A name of the ThreadData field holding the synchronisation
 *   primitive used by the operation (used to exchange data between functions
 *   executed before and after the operation).
 * @param cbtype A type of callback function used to notify the listeners about
 *   the operation.
 * @param cbsparg A function used to transform the synchronisation primitive
 *   used by the operation into a data type expected by the callback function.
 */
#define DEFINE_SYNC_TRAITS(optype, sptype, spfield, cbtype, cbsparg) \
  template<> \
  struct SyncTraits< optype > \
  { \
    typedef sptype SyncPrimitiveType; \
    typedef SyncPrimitiveType ThreadData::*SyncPrimitivePtr; \
    static constexpr SyncPrimitivePtr sp = &ThreadData::spfield; \
    typedef cbtype CallbackType; \
    typedef std::vector< CallbackType > CallbackContainerType; \
    static CallbackContainerType before; \
    static CallbackContainerType after; \
    static constexpr auto sparg = cbsparg; \
  }; \
  SyncTraits< optype >::CallbackContainerType SyncTraits< optype >::before; \
  SyncTraits< optype >::CallbackContainerType SyncTraits< optype >::after;

// Helper sync primitive to callback function argument transformation functions
static inline LOCK getLock(LOCK l) { return l; }
static inline COND getCond(COND c) { return c; }

// Define sync traits information for the supported types of operations
DEFINE_SYNC_TRAITS(ACQUIRE, LOCK, lock, LOCKFUNPTR, &getLock);
DEFINE_SYNC_TRAITS(RELEASE, LOCK, lock, LOCKFUNPTR, &getLock);
DEFINE_SYNC_TRAITS(SIGNAL, COND, cond, CONDFUNPTR, &getCond);
DEFINE_SYNC_TRAITS(WAIT, COND, cond, CONDFUNPTR, &getCond);
DEFINE_SYNC_TRAITS(JOIN, THREAD, thread, JOINFUNPTR, &getThreadId);

/**
 * Notifies all listeners that a thread just performed a synchronisation
 *   operation.
 *
 * @tparam OT A type of the synchronisation operation.
 *
 * @param tid A thread which just performed the synchronisation operation.
 * @param retVal A return value of a function which performed the
 *   synchronisation operation.
 * @param data A structure containing information about a function which
 *   performed the synchronisation operation.
 */
template< SyncOperationType OT >
VOID afterSyncOperation(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // Helper type definitions
  typedef SyncTraits< OT > Traits;
  typedef typename Traits::SyncPrimitiveType SyncPrimitive;

  // Get the sync primitive object used by the operation currently in progress
  SyncPrimitive& obj = g_data.get(tid)->*Traits::sp;

  // Valid sync primitive object means that there is an operation in progress
  assert(obj.is_valid());

  BOOST_FOREACH(typename Traits::CallbackType callback, Traits::after)
  { // Execute all functions to be called after a synchronisation operation
    callback(tid, Traits::sparg(obj));
  }

  obj.invalidate(); // This tells the framework that the operation finished
}

/**
 * Notifies all listeners that a thread is about to perform a synchronisation
 *   operation.
 *
 * @tparam OT A type of the synchronisation operation.
 *
 * @param tid A thread which is about to perform the synchronisation operation.
 * @param sp A value of the stack pointer register.
 * @param arg A pointer to the argument representing a synchronisation primitive
 *   used by the synchronisation operation.
 * @param hi A structure containing information about a function working with
 *   a synchronisation primitive used by the synchronisation operation.
 */
template< SyncOperationType OT >
VOID beforeSyncOperation(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  // Helper type definitions
  typedef SyncTraits< OT > Traits;
  typedef typename Traits::SyncPrimitiveType SyncPrimitive;

  // Call this function when the operation finishes
  if (CALL_AFTER(afterSyncOperation< OT >)) return;

  // Get the object representing the sync primitive used by the operation
  SyncPrimitive obj = mapArgTo< SyncPrimitive >(arg, hi);

  // Sync operations of the same type are non-recursive, so each operation must
  // be finished before it could be started again, if there are no operations
  // in progress (all are finished), the sync primitive object must be invalid
  assert(!(g_data.get(tid)->*Traits::sp).is_valid());

  g_data.get(tid)->*Traits::sp = obj; // Store the sync primitive for later use

  BOOST_FOREACH(typename Traits::CallbackType callback, Traits::before)
  { // Execute all functions to be called before a synchronisation operation
    callback(tid, Traits::sparg(obj));
  }
}

/**
 * Stores information about a created lock.
 *
 * @param tid A thread which just created the lock.
 * @param retVal A return value of the function which just created the lock.
 * @param data A structure containing information about the function which just
 *   created the lock.
 */
VOID afterLockCreate(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // Get the lock created by the function which execution just finished
  LOCK lock = mapArgTo< LOCK >(retVal, static_cast< HookInfo* >(data));

  // Remember that the created object is a lock
  g_objectTypeMap.insert(lock.q(), OT_LOCK);
}

/**
 * Registers a callback function which will be called after a function creates
 *   a lock.
 *
 * @param tid A thread which is about to create the lock.
 * @param sp A value of the stack pointer register.
 * @param hi A structure containing information about the function which is
 *   about to create the lock.
 */
VOID beforeLockCreate(CBSTACK_FUNC_PARAMS, HookInfo* hi)
{
  // Register a callback function to be called after creating the lock
  if (CALL_AFTER(afterLockCreate)) return;
}

/**
 * Gets a type of an object stored at a specific address.
 *
 * @param addr An address at which is the object stored.
 * @param hi A structure containing information about the function working with
 *   the object stored at the specified address.
 * @return The type of the object stored at the specified address.
 */
inline
ObjectType getObjectType(ADDRINT* addr, HookInfo* hi)
{
  for (int depth = hi->refdepth; depth > 0; --depth)
  { // The pointer points to another pointer, not to the data, dereference it
    addr = reinterpret_cast< ADDRINT* >(*addr);
  }

  // Return the type of the object on which is a generic wait function waiting
  return g_objectTypeMap.get(hi->mapper->map(addr));
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
VOID beforeGenericWait(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  switch (getObjectType(arg, hi))
  { // Trigger appropriate notifications based on the type of the object
    case OT_UNKNOWN: // An unknown object, ignore it
      break;
    case OT_LOCK: // A lock, trigger lock acquisition notifications
      beforeSyncOperation< ACQUIRE >(tid, sp, arg, hi);
      break;
    default: // Something is very wrong if the control reaches here
      assert(false);
      break;
  }
}

/**
 * Setups the synchronisation function monitoring, i.e., setups the functions
 *   which will be used for instrumenting the synchronisation functions etc.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupSyncModule(Settings* settings)
{
  BOOST_FOREACH(HookInfo* hi, settings->getHooks())
  { // Setup the functions able to instrument the synchronisation operations
    switch (hi->type)
    { // Configure only synchronisation-related hooks, ignore the others
      case HT_LOCK: // A lock acquired operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeSyncOperation< ACQUIRE >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->lock - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_UNLOCK: // A lock released operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeSyncOperation< RELEASE >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->lock - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_SIGNAL: // A condition signalled operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeSyncOperation< SIGNAL >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->cond - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_WAIT: // A wait for condition operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeSyncOperation< WAIT >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->cond - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_LOCK_INIT: // A lock initialisation operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeLockCreate,
            CBSTACK_IARG_PARAMS,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_GENERIC_WAIT: // A wait for generic object operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeGenericWait,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->object - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      default: // Ignore other hooks
        break;
    }
  }
}

/**
 * Registers a callback function which will be called before acquiring a lock.
 *
 * @param callback A callback function which should be called before acquiring
 *   a lock.
 */
VOID SYNC_BeforeLockAcquire(LOCKFUNPTR callback)
{
  SyncTraits< ACQUIRE >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before releasing a lock.
 *
 * @param callback A callback function which should be called before releasing
 *   a lock.
 */
VOID SYNC_BeforeLockRelease(LOCKFUNPTR callback)
{
  SyncTraits< RELEASE >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before sending a signal.
 *
 * @param callback A callback function which should be called before sending
 *   a signal.
 */
VOID SYNC_BeforeSignal(CONDFUNPTR callback)
{
  SyncTraits< SIGNAL >::before.push_back(callback);
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
  SyncTraits< WAIT >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called after acquiring a lock.
 *
 * @param callback A callback function which should be called after acquiring
 *   a lock.
 */
VOID SYNC_AfterLockAcquire(LOCKFUNPTR callback)
{
  SyncTraits< ACQUIRE >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after releasing a lock.
 *
 * @param callback A callback function which should be called after releasing
 *   a lock.
 */
VOID SYNC_AfterLockRelease(LOCKFUNPTR callback)
{
  SyncTraits< RELEASE >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after sending a signal.
 *
 * @param callback A callback function which should be called after sending
 *   a signal.
 */
VOID SYNC_AfterSignal(CONDFUNPTR callback)
{
  SyncTraits< SIGNAL >::after.push_back(callback);
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
  SyncTraits< WAIT >::after.push_back(callback);
}

/** End of file sync.cpp **/
