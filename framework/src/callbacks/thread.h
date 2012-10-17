/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-10-17
 * @version   0.4
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__

#include <deque>
#include <vector>

#include "pin.H"

#include "../defs.h"
#include "../settings.h"

// Type definitions
typedef std::deque< ADDRINT > Backtrace;
typedef std::vector< std::string > Symbols;

// Definitions of helper functions
VOID setupBacktraceSupport(Settings* settings);

// Definitions of analysis functions (callback functions called by PIN)
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v);

VOID PIN_FAST_ANALYSIS_CALL afterBasePtrPushed(THREADID tid, ADDRINT sp);
VOID PIN_FAST_ANALYSIS_CALL beforeBasePtrPoped(THREADID tid, ADDRINT sp);

VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT idx);
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid);

// Definitions of callback functions
typedef VOID (*THREADFUNPTR)(THREADID tid);

// Definitions of functions for registering callback functions
API_FUNCTION VOID THREAD_ThreadStarted(THREADFUNPTR callback);
API_FUNCTION VOID THREAD_ThreadFinished(THREADFUNPTR callback);

// Definitions of helper functions for retrieving backtraces of threads
API_FUNCTION VOID THREAD_GetLightweightBacktrace(THREADID tid, Backtrace& bt);
API_FUNCTION VOID THREAD_GetPreciseBacktrace(THREADID tid, Backtrace& bt);
API_FUNCTION VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt);

API_FUNCTION VOID THREAD_GetLightweightBacktraceSymbols(Backtrace& bt,
  Symbols& symbols);
API_FUNCTION VOID THREAD_GetPreciseBacktraceSymbols(Backtrace& bt,
  Symbols& symbols);
API_FUNCTION VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
