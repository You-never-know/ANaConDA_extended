/**
 * @brief Contains implementation of a callback stack.
 *
 * A file containing implementation of a stack for storing information about
 *   instrumented function calls.
 *
 * @file      cbstack.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-07
 * @date      Last Update 2016-04-08
 * @version   0.3.1
 */

#include "cbstack.h"

#include <stack>

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
typedef std::stack< Call > CallbackStack;

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

  // There is no after callback function to be called
  if (stack->empty()) return;

  if (stack->top().sp == sp)
  { // We are about to leave (return from) a function which registered an after
    // callback function (we are at the same position in the call stack)
    stack->top().callback(tid, retVal, stack->top().data);
    stack->pop();
  }
}

/**
 * Calls all after callback functions registered to be called after finishing
 *   the execution of the functions which will not finish their execution now
 *   because of a long jump.
 *
 * @note This function is called when the new value of the stack pointer is
 *   known, but before a long jump is performed.
 *
 * @param tid A number identifying the thread which is performing the long jump
 *   (and executing the functions after which the callbacks should be called).
 * @param sp A value of the stack pointer register after the long jump is
 *   performed.
 */
VOID beforeLongJump(THREADID tid, ADDRINT sp)
{
  // Get the callback stack of the thread
  CallbackStack* stack = getCallbackStack(tid);

  while (!stack->empty() && stack->top().sp <= sp)
  { // We are (long) jumping over a function which registered an after callback
    // function (we jumped over the portion of the stack which was used by this
    // function, so it cannot return or continue its execution now). That means
    // that the function just finished its execution without returning (that is
    // why we have no address at which the return value is stored and return 0).
    stack->top().callback(tid, 0, stack->top().data);
    stack->pop();
  }
}

/**
 * Registers an after callback function to be called after the execution of the
 *   current function.
 *
 * @param tid A number identifying the thread which is executing the current
 *   function.
 * @param sp A value of the stack pointer register.
 * @param callback A callback function which should be called.
 * @param data Arbitrary data passed to the callback function.
 * @return If the callback function was successfully registered, the function
 *   will return zero, if a callback function for the current function is
 *   already registered, the function will return @c EREGISTERED.
 */
INT32 registerAfterCallback(CBSTACK_FUNC_PARAMS, CBFUNPTR callback, VOID* data)
{
  if (getCallbackStack(tid)->empty() || getCallbackStack(tid)->top().sp != sp)
  { // No callback function for the current function is registered yet
    getCallbackStack(tid)->push(Call(callback, data, sp));
    return 0;
  }

  // Some callback function already registered
  return EREGISTERED;
}

/** End of file cbstack.cpp **/
