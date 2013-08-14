/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2013-08-14
 * @version   0.11.3
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

// Definitions of functions for configuring thread monitoring
VOID setupThreadModule(Settings* settings);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
