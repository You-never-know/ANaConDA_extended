/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-02-03
 * @version   0.1
 */

#include "thread.h"

namespace
{ // Static global variables (usable only within this module)
  typedef std::vector< THREADFUNPTR > ThreadFunPtrVector;

  ThreadFunPtrVector g_threadStartedVector;
  ThreadFunPtrVector g_threadFinishedVector;
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

/** End of file thread.cpp **/
