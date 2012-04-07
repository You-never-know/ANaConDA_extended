/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-04-07
 * @version   0.2
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__

#include <vector>

#include "pin.H"

// Type definitions
typedef std::vector< ADDRINT > Backtrace;
typedef std::vector< std::string > Symbols;

// Definitions of analysis functions (callback functions called by PIN)
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v);

VOID PIN_FAST_ANALYSIS_CALL beforeRtnExecuted(THREADID tid, ADDRINT sp);

// Definitions of callback functions
typedef VOID (*THREADFUNPTR)(THREADID tid);

// Definitions of functions for registering callback functions
VOID THREAD_ThreadStarted(THREADFUNPTR callback);
VOID THREAD_ThreadFinished(THREADFUNPTR callback);

// Definitions of helper functions for retrieving backtraces of threads
VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt);
VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
