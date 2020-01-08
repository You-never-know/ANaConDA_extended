/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definition of a callback stack.
 *
 * A file containing definition of a stack for storing information about
 *   instrumented function calls.
 *
 * @file      cbstack.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-07
 * @date      Last Update 2016-06-14
 * @version   0.4.1
 */

#ifndef __PINTOOL_ANACONDA__CBSTACK_H__
  #define __PINTOOL_ANACONDA__CBSTACK_H__

#include "pin.H"

/**
 * @brief A helper macro defining all parameters needed by the callback stack
 *   to register an after callback function.
 *
 * @note Parameters defined by this macro need to be the first parameters in
 *   every PIN analysis function which needs to register an after callback
 *   function!
 */
#define CBSTACK_FUNC_PARAMS THREADID tid, ADDRINT sp

/**
 * @brief A helper macro defining all parameters which must be passed to every
 *   PIN analysis function which needs to register an after callback function!
 *
 * @note Parameters defined by this macro need to be the first parameters in
 *   every PIN call insertion function's argument definition block, i.e., @c
 *   InsertCall(<rtn>, <where>, <func>, CBSTACK_IARG_PARAMS, <other-params>,
 *   IARG_END).
 */
#define CBSTACK_IARG_PARAMS \
  IARG_THREAD_ID, \
  IARG_REG_VALUE, REG_STACK_PTR

// Type definitions
typedef VOID (*CBFUNPTR)(THREADID tid, ADDRINT* retVal, VOID* data);

// Definitions of analysis functions (callback functions called by PIN)
VOID createCallbackStack(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

namespace cbstack
{ // Callback functions

VOID beforeReturn(THREADID tid, ADDRINT sp, ADDRINT* retVal);
VOID afterUnwind(THREADID tid, ADDRINT sp);

} // End of namespace cbstack

// Definitions of functions for registering after callback functions
INT32 registerAfterCallback(CBSTACK_FUNC_PARAMS, CBFUNPTR callback, VOID* data);

/**
 * @brief A helper macro simplifying after callback registration.
 *
 * @note The PIN analysis function needs to have the parameters defined by the
 *   @c CBSTACK_FUNC_PARAMS macro as its parameters!
 *
 * @param callback A function which should be called after the execution of the
 *   current function.
 * @param data Arbitrary data passed to the callback function.
 */
#define REGISTER_AFTER_CALLBACK(callback, data) \
  registerAfterCallback(tid, sp, callback, data)

#endif /* __PINTOOL_ANACONDA__CBSTACK_H__ */

/** End of file cbstack.h **/
