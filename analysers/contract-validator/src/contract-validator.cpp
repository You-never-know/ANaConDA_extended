/**
 * @brief An analyser performing dynamic validation of contracts.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of contracts.
 *
 * @file      contract-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-12-11
 * @version   0.2
 */

#include "anaconda.h"

#include "contract.h"

// Type definitions
typedef std::list< FARunner* > CheckedContracts;

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_checkedContractsTlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< CheckedContracts* >(data); }
  );

  std::list< Contract* > g_contracts; //!< A list of loaded contracts.
}

// Helper macros
#define CHECKED_CONTRACTS static_cast< CheckedContracts* >( \\
  TLS_GetThreadData(g_checkedContractsTlsKey, tid))

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID afterLockRelease(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  //
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID threadFinished(THREADID tid)
{
  //
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Register callback functions called before synchronisation events
  SYNC_BeforeLockAcquire(beforeLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_AfterLockRelease(afterLockRelease);

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);

  // Load the contracts
  Contract* contract = new Contract();
  contract->load("contracts");
  g_contracts.push_back(contract);
}

/** End of file contract-validator.cpp **/
