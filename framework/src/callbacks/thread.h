/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-02-03
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__

#include "pin.H"

// Definitions of analysis functions (callback functions called by PIN)
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v);

// Definitions of callback functions
typedef VOID (*THREADFUNPTR)(THREADID tid);

// Definitions of functions for registering callback functions
VOID THREAD_ThreadStarted(THREADFUNPTR callback);
VOID THREAD_ThreadFinished(THREADFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
