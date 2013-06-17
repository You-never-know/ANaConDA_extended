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
 * @date      Last Update 2013-06-17
 * @version   0.1.1
 */

#include "tm.h"

#include <boost/foreach.hpp>

#include "../anaconda.h"

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
    typedef TX##optype##FUNPTR CallbackType; \
    typedef std::vector< CallbackType > CallbackContainerType; \
    static CallbackContainerType before; \
    static CallbackContainerType after; \
  }; \
  TmTraits< optype >::CallbackContainerType TmTraits< optype >::before; \
  TmTraits< optype >::CallbackContainerType TmTraits< optype >::after

// Define TM traits information for the supported types of operations
DEFINE_TM_TRAITS(START);
DEFINE_TM_TRAITS(COMMIT);
DEFINE_TM_TRAITS(ABORT);

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

  BOOST_FOREACH(typename Traits::CallbackType callback, Traits::after)
  { // Execute all functions to be called before a transaction operation
    callback(tid);
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

  BOOST_FOREACH(typename Traits::CallbackType callback, Traits::before)
  { // Execute all functions to be called before a transaction operation
    callback(tid);
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
        // TODO: add support for this operation
        break;
      case HT_TX_WRITE: // A transactional write operation
        // TODO: add support for this operation
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
VOID TM_BeforeTxStart(TXSTARTFUNPTR callback)
{
  TmTraits< START >::before.push_back(callback);
}

/**
 * Registers a function which will be called before committing a transaction.
 *
 * @param callback A function to be called before committing a transaction.
 */
VOID TM_BeforeTxCommit(TXCOMMITFUNPTR callback)
{
  TmTraits< COMMIT >::before.push_back(callback);
}

/**
 * Registers a function which will be called before aborting a transaction.
 *
 * @param callback A function to be called before aborting a transaction.
 */
VOID TM_BeforeTxAbort(TXABORTFUNPTR callback)
{
  TmTraits< ABORT >::before.push_back(callback);
}

/**
 * Registers a function which will be called after starting a transaction.
 *
 * @param callback A function to be called after starting a transaction.
 */
VOID TM_AfterTxStart(TXSTARTFUNPTR callback)
{
  TmTraits< START >::after.push_back(callback);
}

/**
 * Registers a function which will be called after committing a transaction.
 *
 * @param callback A function to be called after committing a transaction.
 */
VOID TM_AfterTxCommit(TXCOMMITFUNPTR callback)
{
  TmTraits< COMMIT >::after.push_back(callback);
}

/**
 * Registers a function which will be called after aborting a transaction.
 *
 * @param callback A function to be called after aborting a transaction.
 */
VOID TM_AfterTxAbort(TXABORTFUNPTR callback)
{
  TmTraits< ABORT >::after.push_back(callback);
}

/** End of file tm.cpp **/
