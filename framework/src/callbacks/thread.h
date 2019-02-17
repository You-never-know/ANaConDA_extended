/*
 * Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Contains definitions of thread-related callback functions.
 *
 * A file containing definitions of callback functions called when some thread
 *   starts or finishes.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2016-06-08
 * @version   0.13.1
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

VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT sp,
  ADDRINT idx);
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionExecuted(THREADID tid, ADDRINT sp,
  ADDRINT idx);
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT sp,
  ADDRINT idx);

// Definitions of functions for configuring thread monitoring
VOID setupThreadModule(Settings* settings);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__THREAD_H__ */

/** End of file thread.h **/
