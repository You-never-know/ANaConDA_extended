/**
 * @brief Contains implementation of functions for monitoring transactional
 *   memory.
 *
 * A file containing implementation of functions for monitoring transactional
 *   memory (TM) operations.
 *
 * @file      tm.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-06
 * @date      Last Update 2013-10-08
 * @version   0.1.5
 */

#include "tm.h"

#include <boost/foreach.hpp>

#include "../anaconda.h"
#include "../cbstack.h"

#include "../utils/tldata.hpp"

/**
 * @brief An enumeration describing the types of transaction operations.
 */
typedef enum TxOperationType_e
{
  START,  //!< A transaction started operation.
  COMMIT, //!< A transaction committed operation.
  ABORT,  //!< A transaction aborted operation.
  READ,   //!< A transactional read operation.
  WRITE   //!< A transactional write operation.
} TxOperationType;

/**
 * @brief A structure containing TM traits information.
 */
template < TxOperationType OT >
struct TmTraits
{
};

/**
 * @brief Defines TM traits information for a specific type of operations.
 *
 * @param optype A type of the operation (constants from the TxOperationType
 *   enumeration).
 */
#define DEFINE_TM_TRAITS(optype) \
  template<> \
  struct TmTraits< optype > \
  { \
    typedef BEFORETX##optype##FUNPTR BeforeCallbackType; \
    typedef AFTERTX##optype##FUNPTR AfterCallbackType; \
    typedef std::vector< BeforeCallbackType > BeforeCallbackContainerType; \
    typedef std::vector< AfterCallbackType > AfterCallbackContainerType; \
    static BeforeCallbackContainerType before; \
    static AfterCallbackContainerType after; \
  }; \
  TmTraits< optype >::BeforeCallbackContainerType TmTraits< optype >::before; \
  TmTraits< optype >::AfterCallbackContainerType TmTraits< optype >::after

// Define TM traits information for the supported types of operations
DEFINE_TM_TRAITS(START);
DEFINE_TM_TRAITS(COMMIT);
DEFINE_TM_TRAITS(ABORT);
DEFINE_TM_TRAITS(READ);
DEFINE_TM_TRAITS(WRITE);

/**
 * @brief A structure holding private data of a thread.
 */
typedef struct ThreadData_s
{
  ADDRINT addr; //!< A memory address accessed from within a transaction.

  /**
   * Constructs a ThreadData_s object.
   */
  ThreadData_s() : addr(0) {}
} ThreadData;

namespace
{ // Static global variables (usable only within this module)
  ThreadLocalData< ThreadData > g_data; //!< Private data of running threads.
}

/**
 * Notifies all listeners that a thread just performed a transaction management
 *   operation, e.g. started or committed a transaction.
 *
 * @tparam OT A type of the transaction management operation. Might be @c START,
 *   @c COMMIT, or @c ABORT.
 *
 * @param tid A thread which just performed a transaction management operation.
 * @param retVal A return value of the function which performed a transaction
 *   management operation.
 * @param data Arbitrary data associated with the call to this function.
 */
template< TxOperationType OT >
VOID afterTxManagementOperation(THREADID tid, ADDRINT* retVal, VOID* data)
{
  typedef TmTraits< OT > Traits; // Here are functions we need to call stored

  BOOST_FOREACH(typename Traits::AfterCallbackType callback, Traits::after)
  { // Execute all functions to be called before a transaction operation
    callback(tid, retVal);
  }
}

/**
 * Notifies all listeners that a thread is about to perform a transaction
 *   management operation, e.g., is about to start or commit a transaction.
 *
 * @tparam OT A type of the transaction management operation. Might be @c START,
 *   @c COMMIT, or @c ABORT.
 *
 * @param tid A thread which is about to perform a transaction management
 *   operation.
 * @param sp A value of the stack pointer register.
 */
template< TxOperationType OT >
VOID beforeTxManagementOperation(CBSTACK_FUNC_PARAMS)
{
  // Register a function to be called after performing a transaction operation
  if (REGISTER_AFTER_CALLBACK(afterTxManagementOperation< OT >, NULL)) return;

  typedef TmTraits< OT > Traits; // Here are functions we need to call stored

  BOOST_FOREACH(typename Traits::BeforeCallbackType callback, Traits::before)
  { // Execute all functions to be called before a transaction operation
    callback(tid);
  }
}

/**
 * Notifies all listeners that a thread just accessed a memory from within a
 *   transaction.
 *
 * @tparam OT A type of the memory access. Might be @c READ or @c WRITE.
 *
 * @param tid A thread which just accessed the memory.
 * @param retVal A return value of the function which accessed the memory.
 * @param data Arbitrary data associated with the call to this function.
 */
template< TxOperationType OT >
VOID afterTxMemoryAccessOperation(THREADID tid, ADDRINT* retVal, VOID* data)
{
  typedef TmTraits< OT > Traits; // Here are functions we need to call stored

  BOOST_FOREACH(typename Traits::AfterCallbackType callback, Traits::after)
  { // Execute all functions to be called before a transaction operation
    callback(tid, g_data.get(tid)->addr);
  }
}

/**
 * Notifies all listeners that a thread is about to access a memory from within
 *   a transaction.
 *
 * @tparam OT A type of the memory access. Might be @c READ or @c WRITE.
 *
 * @param tid A thread which is about to access the memory.
 * @param sp A value of the stack pointer register.
 * @param arg A pointer to the argument containing the accessed memory address.
 * @param hi A structure containing information about a function accessing the
 *   memory.
 */
template< TxOperationType OT >
VOID beforeTxMemoryAccessOperation(CBSTACK_FUNC_PARAMS, ADDRINT* arg,
  HookInfo* hi)
{
  // Register a function to be called after accessing a memory in a transaction
  if (REGISTER_AFTER_CALLBACK(afterTxMemoryAccessOperation< OT >, NULL)) return;

  for (int depth = hi->refdepth; depth > 0; --depth)
  { // The pointer points to another pointer, not to the data, dereference it
    arg = reinterpret_cast< ADDRINT* >(*arg);
  }

  g_data.get(tid)->addr = *arg; // Store data for later use in after callback

  typedef TmTraits< OT > Traits; // Here are functions we need to call stored

  BOOST_FOREACH(typename Traits::BeforeCallbackType callback, Traits::before)
  { // Execute all functions to be called before a transaction operation
    callback(tid, *arg);
  }
}

/**
 * Setups the transactional memory monitoring, i.e., setups the functions which
 *   will be used for instrumenting the transactional memory operations etc.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupTmModule(Settings* settings)
{
  BOOST_FOREACH(HookInfo* hi, settings->getHooks())
  { // Setup the functions able to instrument the TM operations
    switch (hi->type)
    { // Configure only TM-related hooks, ignore the others
      case HT_TX_START: // A transaction started operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeTxManagementOperation< START >,
            CBSTACK_IARG_PARAMS,
            IARG_END);
        };
        break;
      case HT_TX_COMMIT: // A transaction committed operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeTxManagementOperation< COMMIT >,
            CBSTACK_IARG_PARAMS,
            IARG_END);
        };
        break;
      case HT_TX_ABORT: // A transaction aborted operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeTxManagementOperation< ABORT >,
            CBSTACK_IARG_PARAMS,
            IARG_END);
        };
        break;
      case HT_TX_READ: // A transactional read operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeTxMemoryAccessOperation< READ >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->addr - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_TX_WRITE: // A transactional write operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeTxMemoryAccessOperation< WRITE >,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->addr - 1,
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
 * Registers a function which will be called before starting a transaction.
 *
 * @param callback A function to be called before starting a transaction.
 */
VOID TM_BeforeTxStart(BEFORETXSTARTFUNPTR callback)
{
  TmTraits< START >::before.push_back(callback);
}

/**
 * Registers a function which will be called before committing a transaction.
 *
 * @param callback A function to be called before committing a transaction.
 */
VOID TM_BeforeTxCommit(BEFORETXCOMMITFUNPTR callback)
{
  TmTraits< COMMIT >::before.push_back(callback);
}

/**
 * Registers a function which will be called before aborting a transaction.
 *
 * @param callback A function to be called before aborting a transaction.
 */
VOID TM_BeforeTxAbort(BEFORETXABORTFUNPTR callback)
{
  TmTraits< ABORT >::before.push_back(callback);
}

/**
 * Registers a function which will be called before reading from a memory from
 *   within a transaction.
 *
 * @param callback A function to be called before reading from a memory from
 *   within a transaction.
 */
VOID TM_BeforeTxRead(BEFORETXREADFUNPTR callback)
{
  TmTraits< READ >::before.push_back(callback);
}

/**
 * Registers a function which will be called before writing to a memory from
 *   within a transaction.
 *
 * @param callback A function to be called before writing to a memory from
 *   within a transaction.
 */
VOID TM_BeforeTxWrite(BEFORETXWRITEFUNPTR callback)
{
  TmTraits< WRITE >::before.push_back(callback);
}

/**
 * Registers a function which will be called after starting a transaction.
 *
 * @param callback A function to be called after starting a transaction.
 */
VOID TM_AfterTxStart(AFTERTXSTARTFUNPTR callback)
{
  TmTraits< START >::after.push_back(callback);
}

/**
 * Registers a function which will be called after committing a transaction.
 *
 * @param callback A function to be called after committing a transaction.
 */
VOID TM_AfterTxCommit(AFTERTXCOMMITFUNPTR callback)
{
  TmTraits< COMMIT >::after.push_back(callback);
}

/**
 * Registers a function which will be called after aborting a transaction.
 *
 * @param callback A function to be called after aborting a transaction.
 */
VOID TM_AfterTxAbort(AFTERTXABORTFUNPTR callback)
{
  TmTraits< ABORT >::after.push_back(callback);
}

/**
 * Registers a function which will be called after reading from a memory from
 *   within a transaction.
 *
 * @param callback A function to be called after reading from a memory from
 *   within a transaction.
 */
VOID TM_AfterTxRead(AFTERTXREADFUNPTR callback)
{
  TmTraits< READ >::after.push_back(callback);
}

/**
 * Registers a function which will be called after writing to a memory from
 *   within a transaction.
 *
 * @param callback A function to be called after writing to a memory from
 *   within a transaction.
 */
VOID TM_AfterTxWrite(AFTERTXWRITEFUNPTR callback)
{
  TmTraits< WRITE >::after.push_back(callback);
}

/** End of file tm.cpp **/
