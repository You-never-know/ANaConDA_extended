/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2012-11-12
 * @version   0.4.2
 */

#include "thread.h"

#include <assert.h>

#include "../index.h"

#include "../util/rwmap.hpp"

/**
 * Gets a value stored on the stack at a specific address.
 *
 * @param addr An address on the stack at which is the value stored.
 * @return The value stored on the stack at the specified address.
 */
#define STACK_VALUE(addr) *reinterpret_cast< ADDRINT* >(addr)

// Helper macro holding the bottom address of the stack
#define STACK_BOTTOM 0xffffffffffff

// Helper macros
#define CALL_AFTER(callback) \
  REGISTER_AFTER_CALLBACK(callback, static_cast< VOID* >(funcDesc))
#define SET_THREAD_INDEX_CLASS_DATA(tlsKey, indexClass) \
  indexClass* iobj = new indexClass(); \
  iobj->invalidate(); \
  PIN_SetThreadData(tlsKey, iobj, tid);

// Declarations of static functions (usable only within this module)
static VOID deleteBasePointer(void* basePointer);
static VOID deleteBacktrace(void* backtrace);
static VOID deleteJoinedThread(void* joinedThread);

static VOID afterJoin(THREADID tid, ADDRINT* retVal, VOID* data);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_basePointerTlsKey = PIN_CreateThreadDataKey(deleteBasePointer);
  TLS_KEY g_backtraceTlsKey = PIN_CreateThreadDataKey(deleteBacktrace);
  TLS_KEY g_joinedThreadTlsKey = PIN_CreateThreadDataKey(deleteJoinedThread);

  typedef std::vector< THREADFUNPTR > ThreadFunPtrVector;
  typedef std::vector< JOINFUNPTR > JoinFunPtrVector;

  ThreadFunPtrVector g_threadStartedVector;
  ThreadFunPtrVector g_threadFinishedVector;

  JoinFunPtrVector g_beforeJoinVector;

  JoinFunPtrVector g_afterJoinVector;

  typedef VOID (*BACKTRACEFUNPTR)(THREADID tid, Backtrace& bt);
  typedef VOID (*BACKTRACESYMFUNPTR)(Backtrace& bt, Symbols& symbols);

  BACKTRACEFUNPTR g_getBacktraceFunction = NULL;
  BACKTRACESYMFUNPTR g_getBacktraceSymbolsFunction = NULL;

  RWMap< UINT32, THREADID > g_threadIdMap(0);
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
 * Deletes a backtrace.
 *
 * @param backtrace A backtrace.
 */
VOID deleteBacktrace(void* backtrace)
{
  delete static_cast< Backtrace* >(backtrace);
}

/**
 * Deletes a thread object created during thread start.
 *
 * @param joinedThread A thread object.
 */
VOID deleteJoinedThread(void* joinedThread)
{
  delete static_cast< THREAD* >(joinedThread);
}

/**
 * Gets a thread object representing a thread at a specific address.
 *
 * @param threadAddr An address at which is the thread stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the thread at the specified address.
 * @return The thread object representing the thread at the specified address.
 */
inline
THREAD getThread(ADDRINT* threadAddr, FunctionDesc* funcDesc)
{
  for (int lvl = funcDesc->plvl; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the thread, get to it
    threadAddr = reinterpret_cast< ADDRINT* >(*threadAddr);
  }

  // Thread objects must be created in two steps, first create a thread object
  THREAD thread;
  // Then modify it to create a thread object for the specified address
  thread.q_set(funcDesc->farg->map(threadAddr));

  // The created thread must be valid (e.g. the map function cannot return 0)
  assert(thread.is_valid());

  // Return the thread object representing a thread at the specified address
  return thread;
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
 * Gets the current backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The current backtrace of the thread.
 */
Backtrace* getBacktrace(THREADID tid)
{
  return static_cast< Backtrace* >(PIN_GetThreadData(g_backtraceTlsKey, tid));
}

/**
 * Gets a thread object representing the last thread joined with a thread.
 *
 * @param tid A number identifying the thread.
 * @return The thread object representing the last thread joined with the thread.
 */
inline
THREAD* getLastJoinedThread(THREADID tid)
{
  return static_cast< THREAD* >(PIN_GetThreadData(g_joinedThreadTlsKey, tid));
}

/**
 * Setups backtrace retrieval functions based on the type of backtraces the user
 *   want to use.
 *
 * @param settings An object containing framework settings.
 */
VOID setupBacktraceSupport(Settings* settings)
{
  if (settings->get< std::string >("backtrace.type") == "lightweight")
  { // Lightweight: create backtraces on demand by walking the stack
    g_getBacktraceFunction = THREAD_GetLightweightBacktrace;
    g_getBacktraceSymbolsFunction = THREAD_GetLightweightBacktraceSymbols;
  }
  else
  { // Precise: create backtraces on the fly by monitoring calls and returns
    g_getBacktraceFunction = THREAD_GetPreciseBacktrace;
    g_getBacktraceSymbolsFunction = THREAD_GetPreciseBacktraceSymbols;
  }
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
  PIN_SetThreadData(g_backtraceTlsKey, new Backtrace(), tid);
  SET_THREAD_INDEX_CLASS_DATA(g_joinedThreadTlsKey, THREAD);

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
 * Updates a backtrace of a thread. Adds information about the function which
 *   the thread is calling.
 *
 * @note This function is called immediately before a \c CALL instruction is
 *   executed.
 *
 * @param tid A number identifying the thread.
 * @param idx An index of the function which the thread is calling.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT idx)
{
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  CONSOLE("Thread " + decstr(tid) + " is about to execute a call at "
    + retrieveCall(idx) + " [backtrace size is "
    + decstr(getBacktrace(tid)->size()) + "]\n");
#endif
  getBacktrace(tid)->push_front(idx);
}

/**
 * Updates a backtrace of a thread. Removes information about the function from
 *   which is the thread returning.
 *
 * @note This function is called immediately before a \c RETURN instruction is
 *   executed.
 *
 * @param tid A number identifying the thread.
 */
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT idx)
#else
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid)
#endif
{
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  CONSOLE("Thread " + decstr(tid) + " is about to return from a function "
    + retrieveFunction(idx) + " [backtrace size is "
    + decstr(getBacktrace(tid)->size()) + "]\n");
#endif
  assert(!getBacktrace(tid)->empty()); // We can't have more returns than calls

  getBacktrace(tid)->pop_front();
}

/**
 * Creates a mapping between the PIN representation of threads and the concrete
 *   representation of threads used in the multithreading library used.
 *
 * @param tid A thread in which is the thread initialisation function called.
 * @param sp A value of the stack pointer register.
 * @param threadAddr An address at which is the thread stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the thread.
 */
VOID beforeThreadInit(CBSTACK_FUNC_PARAMS, ADDRINT* threadAddr, VOID* funcDesc)
{
  g_threadIdMap.insert(getThread(threadAddr,
    static_cast< FunctionDesc* >(funcDesc)).q(), tid);
}

/**
 * Notifies an analyser that a thread is about to be joined with another thread.
 *
 * @param tid A thread which wants to join with another thread.
 * @param sp A value of the stack pointer register.
 * @param threadAddr An address of a thread which is about to be joind with the
 *   \em tid thread.
 * @param funcDesc A structure containing the description of the function
 *   working with the thread to be joined.
 */
VOID beforeJoin(CBSTACK_FUNC_PARAMS, ADDRINT* threadAddr, VOID* funcDesc)
{
  // Register a callback function to be called after joining the threads
  if (CALL_AFTER(afterJoin)) return;

  // Get the thread stored at the specified address
  THREAD thread = getThread(threadAddr, static_cast< FunctionDesc* >(funcDesc));

  // Cannot enter a join function in the same thread again before leaving it
  assert(!getLastJoinedThread(tid)->is_valid());

  // Save the joined thread for the time when the join function if left
  *getLastJoinedThread(tid) = thread;

  for (JoinFunPtrVector::iterator it = g_beforeJoinVector.begin();
    it != g_beforeJoinVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_threadIdMap.get(thread.q()));
  }
}

/**
 * Notifies an analyser that a thread was joined with another thread.
 *
 * @param tid A thread which wanted to join with another thread.
 * @param retVal A value returned by the join function.
 * @param data An arbitrary data passed to the function.
 */
VOID afterJoin(THREADID tid, ADDRINT* retVal, VOID* data)
{
  // The thread joined must be the last one joined with the thread
  THREAD* thread = getLastJoinedThread(tid);

  // Cannot leave a join function before entering it
  assert(thread->is_valid());

  for (JoinFunPtrVector::iterator it = g_afterJoinVector.begin();
    it != g_afterJoinVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_threadIdMap.get(thread->q()));
  }

  // This will tell the asserts that we left the join function
  thread->invalidate();
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
 * Registers a callback function which will be called before a thread joins with
 *   another thread.
 *
 * @param callback A callback function which should be called before a thread
 *   joins with another thread.
 */
VOID THREAD_BeforeJoin(JOINFUNPTR callback)
{
  g_beforeJoinVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after a thread joins with
 *   another thread.
 *
 * @param callback A callback function which should be called after a thread
 *   joins with another thread.
 */
VOID THREAD_AfterJoin(JOINFUNPTR callback)
{
  g_afterJoinVector.push_back(callback);
}

/**
 * Gets a lightweight backtrace of a thread.
 *
 * @note Lightweight backtraces are created on demand by walking the stack. The
 *   creation might be time-consuming as the whole stack must be processed, but
 *   only the value of the base pointer register needs to be monitored.
 *
 * @param tid A number identifying the thread.
 * @param bt A backtrace containing return addresses present on the stack of the
 *   thread.
 */
VOID THREAD_GetLightweightBacktrace(THREADID tid, Backtrace& bt)
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
 * Gets a precise backtrace of a thread.
 *
 * @note Precise backtraces are created by monitoring \c CALL and \c RETURN
 *   instructions. The monitoring might be time-consuming, but obtaining the
 *   backtrace is quite fast (as it is already available).
 *
 * @param tid A number identifying the thread.
 * @param bt A backtrace containing indexes of function calls.
 */
VOID THREAD_GetPreciseBacktrace(THREADID tid, Backtrace& bt)
{
  bt = *getBacktrace(tid);
}

/**
 * Gets a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @param bt A backtrace.
 */
VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt)
{
  g_getBacktraceFunction(tid, bt);
}

/**
 * Translates return addresses in a lightweight backtrace to strings describing
 *   them.
 *
 * @param bt A backtrace containing return addresses present on the stack of the
 *   thread.
 * @param symbols A vector containing strings describing the return addresses.
 */
VOID THREAD_GetLightweightBacktraceSymbols(Backtrace& bt, Symbols& symbols)
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

/**
 * Translates function call indexes in a precise backtrace to strings describing
 *   them.
 *
 * @param bt A backtrace containing indexes of function calls.
 * @param symbols A vector containing strings describing the indexes of function
 *   calls.
 */
VOID THREAD_GetPreciseBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  for (Backtrace::size_type i = 0; i < bt.size(); i++)
  { // Retrieve the string describing the function call from the index
    symbols.push_back(retrieveCall(bt[i]));
  }
}

/**
 * Translates entries in a backtrace to strings describing them.
 *
 * @param bt A backtrace.
 * @param symbols A vector containing strings describing the entries in the
 *   backtrace.
 */
VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  g_getBacktraceSymbolsFunction(bt, symbols);
}

/** End of file thread.cpp **/
