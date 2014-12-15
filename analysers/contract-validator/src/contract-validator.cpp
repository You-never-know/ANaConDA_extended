/**
 * @brief An analyser performing dynamic validation of contracts.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of contracts.
 *
 * @file      contract-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-12-15
 * @version   0.4
 */

#include "anaconda.h"

#include <boost/foreach.hpp>
#include <boost/regex.hpp>

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
#define CHECKED_CONTRACTS static_cast< CheckedContracts* >( \
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
  TLS_SetThreadData(g_checkedContractsTlsKey, new CheckedContracts(), tid);
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
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID functionEntered(THREADID tid)
{
  // Helper variables
  boost::regex re(".*!([a-zA-Z0-9_]+)\\(.*");
  boost::smatch mo;
  std::string function;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, function);

  // Extract the function name from the signature
  regex_match(function, mo, re);

  // Ignore functions whose names cannot be obtained
  if ((function = mo[1].str()).empty()) return;

  // Helper variables
  FARunner* cc;

  // First try to detect violations in the currently checked contracts
  CheckedContracts::iterator it = CHECKED_CONTRACTS->begin();

  while (it != CHECKED_CONTRACTS->end())
  { // Try to advance to the next function of this contract's method sequence
    if ((*it)->advance(function) && (*it)->accepted())
    { // We got to the end of some method sequence of this contract
      CHECKED_CONTRACTS->erase(it++);

      CONSOLE("Detected contract violation! Contract ends with " + function
        + ".\n");
    }
    else
    { // Next function of this contract is not this function
      ++it;
    }
  }

  // Then add new contracts to be checked if necessary
  BOOST_FOREACH(Contract* contract, g_contracts)
  { // Check if the function is not the first function of some method sequnce
    if ((cc = contract->startsWith(function)) != NULL)
    { // Add this contract to the list of checked contracts
      cc->advance(function);

      CHECKED_CONTRACTS->push_back(cc);
    }
  }
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

  // Register callback functions called when a function is executed
  THREAD_FunctionEntered(functionEntered);

  // Load the contracts
  Contract* contract = new Contract();
  contract->load("contracts");
  g_contracts.push_back(contract);
}

/** End of file contract-validator.cpp **/
