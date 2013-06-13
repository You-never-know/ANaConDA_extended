/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2013-06-13
 * @version   0.10
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__

#include <deque>
#include <vector>

#include "pin.H"

#include "../cbstack.h"
#include "../config.h"
#include "../defs.h"
#include "../index.h"
#include "../settings.h"

// Definitions of classes representing thread primitives
typedef class INDEX< 210 > THREAD; //!< A class representing a thread.

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
template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL afterStackPtrSetByLongJump(THREADID tid, ADDRINT sp);

template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT sp,
  ADDRINT idx);
template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT sp
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  , ADDRINT idx
#endif
  );

template< BacktraceType BT >
VOID beforeThreadCreate(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
VOID beforeThreadInit(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
VOID beforeJoin(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);

// Definitions of internal functions used by other modules
index_t getLastBacktraceLocationIndex(THREADID tid);
std::string getLastBacktraceLocation(THREADID tid);
size_t getBacktraceSize(THREADID tid);

// Definitions of callback functions
typedef VOID (*THREADFUNPTR)(THREADID tid);
typedef VOID (*JOINFUNPTR)(THREADID tid, THREADID jtid);

// Definitions of functions for registering callback functions
API_FUNCTION VOID THREAD_ThreadStarted(THREADFUNPTR callback);
API_FUNCTION VOID THREAD_ThreadFinished(THREADFUNPTR callback);

API_FUNCTION VOID THREAD_BeforeJoin(JOINFUNPTR callback);

API_FUNCTION VOID THREAD_AfterJoin(JOINFUNPTR callback);

// Definitions of functions for retrieving information about threads
API_FUNCTION VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt);
API_FUNCTION VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols);
API_FUNCTION VOID THREAD_GetThreadCreationLocation(THREADID tid,
  std::string& location);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
