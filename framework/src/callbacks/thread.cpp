/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-05-03
 * @version   0.3
 */

#include "thread.h"

/**
 * Gets a value stored on the stack at a specific address.
 *
 * @param addr An address on the stack at which is the value stored.
 * @return The value stored on the stack at the specified address.
 */
#define STACK_VALUE(addr) *reinterpret_cast< ADDRINT* >(addr)

// Helper macro holding the bottom address of the stack
#define STACK_BOTTOM 0xffffffffffff

// Declarations of static functions (usable only within this module)
static VOID deleteBasePointer(void* basePointer);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_basePointerTlsKey = PIN_CreateThreadDataKey(deleteBasePointer);

  typedef std::vector< THREADFUNPTR > ThreadFunPtrVector;

  ThreadFunPtrVector g_threadStartedVector;
  ThreadFunPtrVector g_threadFinishedVector;
}

/**
 * Deletes a value of the base pointer register.
 *
 * @param basePointer A value of the base pointer register.
 */
VOID deleteBasePointer(void* basePointer)
{
  delete static_cast< ADDRINT* >(basePointer);
}

/**
 * Gets the last value of the base pointer register of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The last value of the base pointer register of the thread.
 */
inline
ADDRINT* getBasePointer(THREADID tid)
{
  return static_cast< ADDRINT* >(PIN_GetThreadData(g_basePointerTlsKey, tid));
}

/**
 * Calls all callback functions registered by a user to be called when a thread
 *   starts.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID threadStarted(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // Allocate memory for storing the last value of the thread's base pointer
  PIN_SetThreadData(g_basePointerTlsKey, new ADDRINT(0), tid);

  for (ThreadFunPtrVector::iterator it = g_threadStartedVector.begin();
    it != g_threadStartedVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid);
  }
}

/**
 * Calls all callback functions registered by a user to be called when a thread
 *   finishes.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the final register state of the thread.
 * @param flags OS specific termination code.
 * @param v Data passed to the callback registration function.
 */
VOID threadFinished(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v)
{
  for (ThreadFunPtrVector::iterator it = g_threadFinishedVector.begin();
    it != g_threadFinishedVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid);
  }
}

/**
 * Stores a value of the base pointer register of a thread.
 *
 * @note This function is called immediately after a \c PUSH instruction which
 *   pushes the value of the base pointer register onto a stack.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
VOID PIN_FAST_ANALYSIS_CALL afterBasePtrPushed(THREADID tid, ADDRINT sp)
{
  // The stack pointer now points to the previous value of the base pointer
  // stored on the top of the stack, because the base pointer will be updated
  // to point the same location a the stack pointer in a while, we can store
  // the value of the stack pointer as the value of the updated base pointer
  *getBasePointer(tid) = sp;
}

/**
 * Stores a value of the base pointer register of a thread.
 *
 * @note This function is called immediately before a \c POP instruction which
 *   pops the previous value of the base pointer into the base pointer register
 *   or before a \c LEAVE instruction which does the same thing.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeBasePtrPoped(THREADID tid, ADDRINT sp)
{
  // The value of the previous base pointer is on the top of the stack or where
  // the base pointer register points (in case of LEAVE, which passes the value
  // of the base pointer register instead of the stack pointer register to this
  // function). We need to check if the previous base pointer seems to be valid,
  // i.e., if its value is higher than the value of the stack pointer, so we are
  // (probably) backtracking to the previous stack frames. If we store the value
  // without this check and the value is not valid, it may cause a segmentation
  // fault when we try to unwind the stack frames later.
  *getBasePointer(tid) = (STACK_VALUE(sp) > sp) ? STACK_VALUE(sp) : 0;
}

/**
 * Registers a callback function which will be called when a thread starts.
 *
 * @param callback A callback function which should be called when a thread
 *   starts.
 */
VOID THREAD_ThreadStarted(THREADFUNPTR callback)
{
  g_threadStartedVector.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread finishes.
 *
 * @param callback A callback function which should be called when a thread
 *   finishes.
 */
VOID THREAD_ThreadFinished(THREADFUNPTR callback)
{
  g_threadFinishedVector.push_back(callback);
}

/**
 * Gets a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @param bt A vector (backtrace) containing return addresses present on the
 *   stack of the thread.
 */
VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt)
{
  // Get the last value of the base pointer
  ADDRINT bp = *getBasePointer(tid);

  while (bp != 0)
  { // Stack frame validity checks: we must backtrack to the bottom of the stack
    // until we reach zero, which means we unwound all stack frames and are done
    // (the value of the previous base pointer must be between the values of the
    // current base pointer and the bottom of the stack, if it is not zero). If
    // any of this requirements is violated, we stop the unwind process as the
    // frame is definitely not valid
    if ((STACK_VALUE(bp) < bp && STACK_VALUE(bp) != 0)
      || (STACK_VALUE(bp) > STACK_BOTTOM)) return;
    // Return address is stored under the value of the previous base pointer
    bt.push_back(*(ADDRINT*)(bp + sizeof(ADDRINT)));
    // Backtrack to the previous stack frame (get the previous base pointer)
    bp = *(ADDRINT*)(bp);
  }
}

/**
 * Translates return addresses in a backtrace to strings describing them.
 *
 * @param bt A vector (backtrace) containing return addresses present on the
 *   stack of the thread.
 * @param symbols A vector containing strings describing the addresses.
 */
VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  // Helper variables
  std::string file;
  INT32 line;

  // Locking the client for the whole loop is more effective then in each loop
  PIN_LockClient();

  for (Backtrace::size_type i = 0; i < bt.size(); i++)
  { // Get the source code location for the return address in the backtrace
    PIN_GetSourceLocation(bt[i], NULL, &line, &file);

    // Symbol format: <filename>:<line number> [<return address>]
    symbols.push_back((file.empty() ? "<unknown>" : file + ":" + decstr(line))
      + " [" + hexstr(bt[i]) + "]");
  }

  // All return addresses translated
  PIN_UnlockClient();
}

/** End of file thread.cpp **/
