/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-04-07
 * @version   0.2
 */

#include "thread.h"

// Declarations of static functions (usable only within this module)
static VOID deleteBasePointer(void* basePointer);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_basePointerTlsKey = PIN_CreateThreadDataKey(deleteBasePointer);

  typedef std::vector< THREADFUNPTR > ThreadFunPtrVector;

  ThreadFunPtrVector g_threadStartedVector;
  ThreadFunPtrVector g_threadFinishedVector;
}

/**
 * Deletes a value of the base pointer register.
 *
 * @param basePointer A value of the base pointer register.
 */
VOID deleteBasePointer(void* basePointer)
{
  delete static_cast< ADDRINT* >(basePointer);
}

/**
 * Gets the last value of the base pointer register of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The last value of the base pointer register of the thread.
 */
inline
ADDRINT* getBasePointer(THREADID tid)
{
  return static_cast< ADDRINT* >(PIN_GetThreadData(g_basePointerTlsKey, tid));
}

/**
 * Calls all callback functions registered by a user to be called when a thread
 *   starts.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // Allocate memory for storing the last value of the thread's base pointer
  PIN_SetThreadData(g_basePointerTlsKey, new ADDRINT, tid);

  for (ThreadFunPtrVector::iterator it = g_threadStartedVector.begin();
    it != g_threadStartedVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid);
  }
}

/**
 * Calls all callback functions registered by a user to be called when a thread
 *   finishes.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the final register state of the thread.
 * @param flags OS specific termination code.
 * @param v Data passed to the callback registration function.
 */
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v)
{
  for (ThreadFunPtrVector::iterator it = g_threadFinishedVector.begin();
    it != g_threadFinishedVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid);
  }
}

/**
 * Stores a value of the base pointer register of a thread.
 *
 * @note This function is called immediately after a CALL instruction.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeRtnExecuted(THREADID tid, ADDRINT sp)
{
  // To get a backtrace, we need to chase the base pointers under which are the
  // return addresses we need to get stored, but before a function is executed,
  // the base pointer is not updated yet (it points above the previous return
  // address and not above the current return address), but the stack pointer
  // now points to the current return address, so we can compute the value of
  // the base pointer from the value of the stack pointer we know here
  *getBasePointer(tid) = sp - sizeof(ADDRINT);
}

/**
 * Registers a callback function which will be called when a thread starts.
 *
 * @param callback A callback function which should be called when a thread
 *   starts.
 */
VOID THREAD_ThreadStarted(THREADFUNPTR callback)
{
  g_threadStartedVector.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread finishes.
 *
 * @param callback A callback function which should be called when a thread
 *   finishes.
 */
VOID THREAD_ThreadFinished(THREADFUNPTR callback)
{
  g_threadFinishedVector.push_back(callback);
}

/**
 * Gets a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @param bt A vector (backtrace) containing return addresses present on the
 *   stack of the thread.
 */
VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt)
{
  // Get the last value of the base pointer
  ADDRINT bp = *getBasePointer(tid);

  while (bp != 0)
  { // Return address is stored under the value of the previous base pointer
    bt.push_back(*(ADDRINT*)(bp + sizeof(ADDRINT)));
    // Backtrack to the previous stack frame (get the previous base pointer)
    bp = *(ADDRINT*)(bp);
  }
}

/**
 * Translates return addresses in a backtrace to strings describing them.
 *
 * @param bt A vector (backtrace) containing return addresses present on the
 *   stack of the thread.
 * @param symbols A vector containing strings describing the addresses.
 */
VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  // Helper variables
  std::string file;
  INT32 line;

  // Locking the client for the whole loop is more effective then in each loop
  PIN_LockClient();

  for (Backtrace::size_type i = 0; i < bt.size(); i++)
  { // Get the source code location for the return address in the backtrace
    PIN_GetSourceLocation(bt[i], NULL, &line, &file);

    // Symbol format: <filename>:<line number> [<return address>]
    symbols.push_back((file.empty() ? "<unknown>" : file + ":" + decstr(line))
      + " [" + hexstr(bt[i]) + "]");
  }

  // All return addresses translated
  PIN_UnlockClient();
}

/** End of file thread.cpp **/
