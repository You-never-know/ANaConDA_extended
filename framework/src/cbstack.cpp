/**
 * @brief Contains implementation of a callback stack.
 *
 * A file containing implementation of a stack for storing information about
 *   instrumented function calls.
 *
 * @file      cbstack.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-07
 * @date      Last Update 2012-02-08
 * @version   0.1
 */

#include "cbstack.h"

#include <stack>

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
   * @brief A value of the stack pointer just after calling the instrumented
   *   function, i.e., when the code of the instrumented function is about to
   *   execute.
   */
  ADDRINT sp;

  /**
   * Constructs a Call_s object.
   */
  Call_s() : callback(NULL), sp(0) {}

  /**
   * Constructs a Call_s object.
   *
   * @param c A function which should be called after executing an instrumented
   *   function.
   * @param s A value of the stack pointer just after the instrumented function
   *   call.
   */
  Call_s(CBFUNPTR c, ADDRINT s) : callback(c), sp(s) {}
} Call;

// Type definitions
typedef std::stack< Call > CallbackStack;

// Declarations of static functions (usable only within this module)
static VOID deleteCallbackStack(void* stack);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_callbackStackTlsKey = PIN_CreateThreadDataKey(deleteCallbackStack);
}

/**
 * Deletes a callback stack created during thread start.
 *
 * @param stack A callback stack.
 */
VOID deleteCallbackStack(void* stack)
{
  delete static_cast< CallbackStack* >(stack);
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
 */
VOID beforeReturn(CBSTACK_FUNC_PARAMS)
{
  // Get the callback stack of the thread
  CallbackStack* stack = getCallbackStack(tid);

  // There is no after callback function to be called
  if (stack->empty()) return;

  if (stack->top().sp == sp)
  { // We are about to leave (return from) a function which registered an after
    // callback function (we are at the same position in the call stack)
    stack->top().callback(tid);
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
 */
VOID registerAfterCallback(CBSTACK_FUNC_PARAMS, CBFUNPTR callback)
{
  getCallbackStack(tid)->push(Call(callback, sp));
}

/** End of file cbstack.cpp **/
