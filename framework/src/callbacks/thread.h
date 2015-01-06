/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2014-12-19
 * @version   0.12.2
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__

#include "pin.H"

#include "../config.h"
#include "../settings.h"

// Definitions of analysis functions (callback functions called by PIN)
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v);

VOID PIN_FAST_ANALYSIS_CALL afterBasePtrPushed(THREADID tid, ADDRINT sp);
VOID PIN_FAST_ANALYSIS_CALL beforeBasePtrPoped(THREADID tid, ADDRINT sp);
VOID PIN_FAST_ANALYSIS_CALL afterStackPtrSetByLongJump(THREADID tid, ADDRINT sp);

VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT sp,
  ADDRINT idx);
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionExecuted(THREADID tid, ADDRINT sp,
  ADDRINT idx);
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT sp
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  , ADDRINT idx
#endif
  );

// Definitions of functions for configuring thread monitoring
VOID setupThreadModule(Settings* settings);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
