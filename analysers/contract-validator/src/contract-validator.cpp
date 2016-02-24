/**
 * @brief An analyser performing dynamic validation of contracts.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of contracts.
 *
 * @file      contract-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-24
 * @version   0.3.2
 */

#include "anaconda.h"

#include <regex>

#include <boost/filesystem/fstream.hpp>

#include "contract.h"
#include "vc.hpp"

// Namespace aliases
namespace fs = boost::filesystem;

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    VectorClock cvc; //!< The current vector clock of the thread.

    /**
     * Constructs a ThreadData_s object.
     *
     * @param tid A thread owning the data.
     */
    ThreadData_s(THREADID tid)
    {
      cvc.init(tid); // Initialise the current vector clock of the thread
    }
  } ThreadData;

  // A key for accessing private data of a thread in the Thread Local Storage
  TLS_KEY g_tlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< ThreadData* >(data); }
  );

  std::list< Contract* > g_contracts; //!< A list of contracts to be checked.

  std::map< LOCK, VectorClock > g_locks; //!< Vector clocks for locks (L).
  PIN_RWMUTEX g_locksLock; //!< A lock guarding access to @c g_locks map.
}

// A helper macro for accessing the Thread Local Storage (TLS) more easily
#define TLS static_cast< ThreadData* >(TLS_GetThreadData(g_tlsKey, tid))

/**
 * Gets a name of a function currently being executed by a specific thread.
 *
 * @param tid A number identifying the thread.
 * @param name A name of the function currently being executed by the thread @em
 *   tid.
 * @return @em True if the name was obtained successfully, @em False if the name
 *   could not be obtained or is empty.
 */
inline
BOOL getCurrentFunctionName(THREADID tid, std::string& name)
{
  // Helper variables
  std::string signature;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, signature);

  // The obtained signature has the following format: <module>!<function>
  std::regex re(".*!([a-zA-Z0-9_:]+)");
  std::smatch mo;

  // Extract the function name from the signature
  regex_match(signature, mo, re);

  // Ignore functions whose names cannot be obtained
  if ((name = mo[1].str()).empty()) return false;

  return true; // Name of the function obtained successfully
}

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
  // As only a single thread may release a specific lock at one time, different
  // threads cannot insert items with the same key concurrently, however, it is
  // possible that two threads may insert items with different keys at the same
  // time and we need to do it safely (write lock gives us exclusive access)
  PIN_RWMutexWriteLock(&g_locksLock);
  g_locks[lock] = TLS->cvc; // L_lock' = C_tid
  PIN_RWMutexUnlock(&g_locksLock);

  TLS->cvc.increment(tid); // C_tid' = inc_tid(C_tid)
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  // We are only reading information here, so read lock is sufficient for us
  PIN_RWMutexReadLock(&g_locksLock);
  // TODO: based on the documentation, it should be safe to read other items
  // in the map concurrently with the insertion of new items, however, it is
  // still questionable as std::map uses RB trees which may need rebalancing

  try
  { // We can only read here, so we cannot use operator[] to access the data
    TLS->cvc.join(g_locks.at(lock)); // C_tid' = C_tid join L_lock
  }
  catch (std::out_of_range& e) {}

  PIN_RWMutexUnlock(&g_locksLock);
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
  TLS_SetThreadData(g_tlsKey, new ThreadData(tid), tid);
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
  std::string function;

  // Do not continue if the name of the entered function cannot be obtained
  if (!getCurrentFunctionName(tid, function)) return;

  CONSOLE("Thread " + decstr(tid) + ": ENTER: " + function + ", vc: " + TLS->cvc
    + "\n");
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID functionExited(THREADID tid)
{
  // Helper variables
  std::string function;

  // Do not continue if the name of the exited function cannot be obtained
  if (!getCurrentFunctionName(tid, function)) return;

  CONSOLE("Thread " + decstr(tid) + ": EXIT: " + function + ", vc: " + TLS->cvc
    + "\n");
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Initialise locks
  PIN_RWMutexInit(&g_locksLock);

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
  THREAD_FunctionExited(functionExited);

  // Load the contracts to be checked
  Contract* contract = new Contract();
  contract->load("contracts");
  g_contracts.push_back(contract);

  // Dump the loaded contracts
  fs::ofstream f("contracts.dump");
  f << contract->toString();
  f.close();
}

/**
 * Cleans up the analyser.
 */
PLUGIN_FINISH_FUNCTION()
{
  // Free locks
  PIN_RWMutexFini(&g_locksLock);

  for (Contract* contract : g_contracts)
  { // Free all loaded contracts
    delete contract;
  }
}

/** End of file contract-validator.cpp **/
