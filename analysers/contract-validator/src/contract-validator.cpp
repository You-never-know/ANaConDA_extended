/**
 * @brief An analyser performing dynamic validation of contracts.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of contracts.
 *
 * @file      contract-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-18
 * @version   0.1
 */

#include "anaconda.h"

#include <regex>

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    /**
     * Constructs a ThreadData_s object.
     */
    ThreadData_s() {}
  } ThreadData;

  // A key for accessing private data of a thread in the Thread Local Storage
  TLS_KEY g_tlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< ThreadData* >(data); }
  );
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
  TLS_SetThreadData(g_tlsKey, new ThreadData(), tid);
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

  CONSOLE("Thread " + decstr(tid) + ": ENTER: " + function + "\n");
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

  CONSOLE("Thread " + decstr(tid) + ": EXIT: " + function + "\n");
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
  THREAD_FunctionExited(functionExited);
}

/**
 * Cleans up the analyser.
 */
PLUGIN_FINISH_FUNCTION()
{
  //
}

/** End of file contract-validator.cpp **/
