/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of a callback stack.
 *
 * A file containing implementation of a stack for storing information about
 *   instrumented function calls.
 *
 * @file      cbstack.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-07
 * @date      Last Update 2019-02-17
 * @version   0.4.5
 */

#include "cbstack.h"

#include <deque>

#include "defs.h"

/**
 * @brief A structure containing information about an instrumented call.
 */
typedef struct Call_s
{
  /**
   * @brief A function which should be called after executing the instrumented
   *   function. This function will be called just before executing the return
   *   instruction for the instrumented function.
   */
  CBFUNPTR callback;
  /**
   * @brief Arbitrary data passed to the callback function.
   */
  VOID* data;
  /**
   * @brief A value of the stack pointer just after calling the instrumented
   *   function, i.e., when the code of the instrumented function is about to
   *   execute.
   */
  ADDRINT sp;

  /**
   * Constructs a Call_s object.
   */
  Call_s() : callback(NULL), data(NULL), sp(0) {}

  /**
   * Constructs a Call_s object.
   *
   * @param c A function which should be called after executing an instrumented
   *   function.
   * @param d Arbitrary data passed to the callback function.
   * @param s A value of the stack pointer just after the instrumented function
   *   call.
   */
  Call_s(CBFUNPTR c, VOID* d, ADDRINT s) : callback(c), data(d), sp(s) {}
} Call;

// Type definitions
typedef std::deque< Call > CallbackStack;

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_callbackStackTlsKey = PIN_CreateThreadDataKey(
    [] (VOID* stack) { delete static_cast< CallbackStack* >(stack); }
  );
}

/**
 * Gets a callback stack of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The callback stack associated with the specified thread.
 */
inline
CallbackStack* getCallbackStack(THREADID tid)
{
  return static_cast< CallbackStack* >(PIN_GetThreadData(g_callbackStackTlsKey,
    tid));
}

/**
 * Creates a callback stack for a thread.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID createCallbackStack(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // Create a callback stack and store it in the TLS of the created thread
  PIN_SetThreadData(g_callbackStackTlsKey, new CallbackStack(), tid);
}

namespace cbstack
{ // Callback functions

/**
 * Calls an after callback function if there is one registered to be called
 *   after the execution of the current function.
 *
 * @param tid A number identifying the thread which is executing the current
 *   function.
 * @param sp A value of the stack pointer register.
 * @param retVal A value returned by the current function.
 */
VOID beforeReturn(THREADID tid, ADDRINT sp, ADDRINT* retVal)
{
  // Get the callback stack of the thread
  CallbackStack* stack = getCallbackStack(tid);

  while (!stack->empty() && stack->back().sp < sp)
  { // We must have missed some return(s) as the stack frame for the call that
    // is on the top of the call stack was already destroyed, so the function
    // called has already finished its execution, issue the notification now
    CONSOLE_NOPREFIX("W: Missed return of call at SP "
      + hexstr(stack->back().sp)
      + ", calling registered callback function now.\n");

    // The return value of the call was already lost, so return NULL
    stack->back().callback(tid, 0, stack->back().data);
    stack->pop_back();
  }

  while (!stack->empty() && stack->back().sp == sp)
  { // We are about to leave (return from) a function which registered an after
    // callback function (we are at the same position in the call stack)
    stack->back().callback(tid, retVal, stack->back().data);
    stack->pop_back();
  }
}

/**
 * Calls the callback functions for all functions the program is returning from
 *   by unwinding their portion of the call stack.
 *
 * When unwinding the stack, the program is returning from several functions at
 *   once. These functions will not end normally now (by calling return), still
 *   they ended their execution and there may be some registered callbacks that
 *   needs to be called when they finish their execution. Therefore, we need to
 *   call all of these callback functions here.
 *
 * @note This function is called immediately after an unwind function finishes
 *   unwinding the stack. Actually, it is called right after an instruction in
 *   the unwind function sets the new value of the stack pointer.
 *
 * @param tid A number identifying the thread whose stack is being unwinded.
 * @param sp A new value of the stack pointer register of the thread.
 */
VOID afterUnwind(THREADID tid, ADDRINT sp)
{
  // Get the callback stack of the thread
  CallbackStack* stack = getCallbackStack(tid);

  while (!stack->empty() && stack->back().sp <= sp)
  { // We are (long) jumping over a function which registered an after callback
    // function (we jumped over the portion of the stack which was used by this
    // function, so it cannot return or continue its execution now). That means
    // that the function just finished its execution without returning (that is
    // why we have no address at which the return value is stored and return 0).
    stack->back().callback(tid, 0, stack->back().data);
    stack->pop_back();
  }
}

} // End of namespace cbstack

/**
 * Registers an after callback function to be called after the execution of the
 *   current function (identified by the value of the stack pointer).
 *
 * @note For each executing function (value of SP), the same callback function
 *   can be registered only once. In other words, different callback functions
 *   can be registered for the same value of the stack pointer, however, it is
 *   not possible to register the same callback function for the same value of
 *   stack pointer twice.
 *
 * @param tid A number identifying the thread which is executing the current
 *   function.
 * @param sp A value of the stack pointer register identifying the currently
 *   executing function.
 * @param callback A callback function which will be called after the
 *   currently executing function finishes its execution.
 * @param data Arbitrary data passed to the callback function.
 * @return @c 0 if the callback function registered successfully, @c EREGISTERED
 *   if the callback function is already registered (for the value of the stack
 *   pointer specified).
 */
INT32 registerAfterCallback(CBSTACK_FUNC_PARAMS, CBFUNPTR callback, VOID* data)
{
  // Get the callback stack of the thread
  CallbackStack* stack = getCallbackStack(tid);

  // First check if the callback function is not already registered
  CallbackStack::reverse_iterator it = stack->rbegin();

  // For the same SP each callback function can be registered only once
  while (it != stack->rend())
  { // If the SPs are different, we can register the callback function
    if (it->sp != sp) break;

    // The callback function specified is already registered for the SP
    if (it->callback == callback) return EREGISTERED;

    ++it; // Same SP, different callback, move to the next call entry
  }

  // The callback function specified is not registered for this SP yet
  stack->push_back(Call(callback, data, sp));

  return 0; // Registration successful
}

/** End of file cbstack.cpp **/
