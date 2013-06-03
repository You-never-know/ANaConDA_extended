/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2013-06-03
 * @version   0.7
 */

#include "thread.h"

#include <assert.h>

#include <boost/foreach.hpp>

#include "../monitors/preds.h"

#include "../utils/backtrace.hpp"
#include "../utils/rwmap.hpp"

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
#define THREAD_DATA getThreadData(tid)

// Declarations of static functions (usable only within this module)
static VOID deleteThreadData(void* threadData);

template < BacktraceType BTT >
static VOID afterThreadCreate(THREADID tid, ADDRINT* retVal, VOID* data);
static VOID afterJoin(THREADID tid, ADDRINT* retVal, VOID* data);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_threadDataTlsKey = PIN_CreateThreadDataKey(deleteThreadData);

  typedef std::vector< std::pair< THREADINITFUNPTR, VOID* > >
    ThreadInitFunctionContainerType;

  ThreadInitFunctionContainerType g_threadInitFunctions;

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
  RWMap< UINT32, std::string > g_threadCreateLocMap("<unknown>");

  PredecessorsMonitor< FileWriter >* g_predsMon;
}

// Type definitions
typedef std::vector< ADDRINT > BtSpVector;

/**
 * @brief A structure holding private data of a thread.
 */
typedef struct ThreadData_s
{
  ADDRINT bp; //!< A value of the thread's base pointer register.
  Backtrace backtrace; //!< The current backtrace of a thread.
  BtSpVector btsplist; //!< The values of stack pointer of calls in backtrace.
  THREAD ljthread; //!< The last thread joined with a thread.
  std::string tcloc; //!< A location where a thread was created.
  ADDRINT arg; //!< A value of an argument of a function called by a thread.

  /**
   * Constructs a ThreadData_s object.
   */
  ThreadData_s() : bp(0), backtrace(), btsplist(), ljthread(),
    tcloc("<unknown>"), arg()
  {
    // Do not assume that the default constructor will invalidate the object
    ljthread.invalidate();
  }
} ThreadData;

/**
 * Deletes an object holding private data of a thread.
 *
 * @param threadData An object holding private data of a thread.
 */
VOID deleteThreadData(void* threadData)
{
  delete static_cast< ThreadData* >(threadData);
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
 * Gets an object holding private data of a thread.
 *
 * @param tid A number identifying the thread.
 * @return An object holding private data of the thread.
 */
ThreadData* getThreadData(THREADID tid)
{
  return static_cast< ThreadData* >(PIN_GetThreadData(g_threadDataTlsKey, tid));
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
VOID getLightweightBacktrace(THREADID tid, Backtrace& bt)
{
  // Get the last value of the base pointer
  ADDRINT bp = THREAD_DATA->bp;

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
VOID getPreciseBacktrace(THREADID tid, Backtrace& bt)
{
  bt = THREAD_DATA->backtrace;
}

/**
 * Translates return addresses in a lightweight backtrace to strings describing
 *   them.
 *
 * @tparam BV Determines how detailed the locations in the backtrace will be.
 *
 * @param bt A backtrace containing return addresses present on the stack of the
 *   thread.
 * @param symbols A vector containing strings describing the return addresses.
 */
template < BacktraceVerbosity BV >
VOID getLightweightBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  for (Backtrace::size_type i = 0; i < bt.size(); i++)
  { // Get the source code location for the return address in the backtrace
    symbols.push_back(makeBacktraceLocation< BV, FI_LOCKED >(bt[i]));
  }
}

/**
 * Translates function call indexes in a precise backtrace to strings describing
 *   them.
 *
 * @param bt A backtrace containing indexes of function calls.
 * @param symbols A vector containing strings describing the indexes of function
 *   calls.
 */
VOID getPreciseBacktraceSymbols(Backtrace& bt, Symbols& symbols)
{
  for (Backtrace::size_type i = 0; i < bt.size(); i++)
  { // Retrieve the string describing the function call from the index
    symbols.push_back(retrieveCall(bt[i]));
  }
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
    g_getBacktraceFunction = getLightweightBacktrace;

    if (settings->get< std::string >("backtrace.verbosity") == "minimal")
    { // Minimal: locations only
      g_getBacktraceSymbolsFunction = getLightweightBacktraceSymbols< BV_MINIMAL >;
    }
    else
    { // Detailed: names of images and functions + locations
      g_getBacktraceSymbolsFunction = getLightweightBacktraceSymbols< BV_DETAILED >;
    }
  }
  else
  { // Precise: create backtraces on the fly by monitoring calls and returns
    g_getBacktraceFunction = getPreciseBacktrace;
    g_getBacktraceSymbolsFunction = getPreciseBacktraceSymbols;
  }

  g_predsMon = &settings->getCoverageMonitors().preds;
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
  // Allocate memory for storing private data of the starting thread
  PIN_SetThreadData(g_threadDataTlsKey, new ThreadData(), tid);

  BOOST_FOREACH(ThreadInitFunctionContainerType::const_reference item,
    g_threadInitFunctions)
  { // Call all thread initialisation functions, stored as (func, data) pairs
    item.first(tid, item.second);
  }

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
  THREAD_DATA->bp = sp;
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
  THREAD_DATA->bp = (STACK_VALUE(sp) > sp) ? STACK_VALUE(sp) : 0;
}

/**
 * Backtracks in a backtrace of a thread to a call which executed the function
 *   where the program is jumping using a long jump.
 *
 * @note This function is called immediately after an instruction in a long jump
 *   routine restores the value of the stack pointer.
 *
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL afterStackPtrSetByLongJump(THREADID tid, ADDRINT sp)
{
  // As we are monitoring function calls, we are not returning to the function
  // to which is the long jump jumping, but to the call to this function. This
  // means that if the stored SP is equal to the SP where the long jump is
  // jumping, it is the call from the function to which we are jumping and we
  // need to delete this call from the backtrace too
  while (THREAD_DATA->btsplist.back() <= sp)
  { // Backtrack to the call which executed the function where we are jumping
    THREAD_DATA->backtrace.pop_front();
    THREAD_DATA->btsplist.pop_back();

    if (CC & CC_PREDS)
    { // Notify the monitor that we are leaving a function
      g_predsMon->beforeFunctionExited(tid);
    }
  }
}

/**
 * Updates a backtrace of a thread. Adds information about the function which
 *   the thread is calling.
 *
 * @note This function is called immediately before a \c CALL instruction is
 *   executed.
 *
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 * @param idx An index of the function which the thread is calling.
 */
template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT sp,
  ADDRINT idx)
{
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  CONSOLE("Thread " + decstr(tid) + " is about to execute a call at "
    + retrieveCall(idx) + " [backtrace size is "
    + decstr(THREAD_DATA->backtrace.size()) + "]\n");
#endif
  if (!THREAD_DATA->btsplist.empty())
    if (THREAD_DATA->btsplist.back() < sp)
      WARNING("Previous value of SP [" + hexstr(
        (BtSpVector::value_type)THREAD_DATA->btsplist.back())
        + "] is lower than the current value of SP [" + hexstr(sp) + "]\n");

  // Add the call to be executed to the backtrace
  THREAD_DATA->backtrace.push_front(idx);
  THREAD_DATA->btsplist.push_back(sp);

  if (CC & CC_PREDS)
  { // Notify the monitor that we are entering a function
    g_predsMon->beforeFunctionEntered(tid);
  }
}

/**
 * Updates a backtrace of a thread. Removes information about the function from
 *   which is the thread returning.
 *
 * @note This function is called immediately before a \c RETURN instruction is
 *   executed.
 *
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
template < ConcurrentCoverage CC >
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT sp
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  , ADDRINT idx
#endif
  )
{
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  CONSOLE("Thread " + decstr(tid) + " is about to return from a function "
    + retrieveFunction(idx) + " [backtrace size is "
    + decstr(THREAD_DATA->backtrace.size()) + "]\n");
#endif
  // We can't have more returns than calls
  assert(!THREAD_DATA->backtrace.empty());

  // Return to the call which executed the function where we are returning
  THREAD_DATA->backtrace.pop_front();
  THREAD_DATA->btsplist.pop_back();

  if (CC & CC_PREDS)
  { // Notify the monitor that we are leaving a function
    g_predsMon->beforeFunctionExited(tid);
  }
}

// Helper macros
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
  #define BFR_ADDITIONAL_PARAMS , ADDRINT idx
#else
  #define BFR_ADDITIONAL_PARAMS
#endif

/**
 * Instantiates a concrete code of function execution functions from a template.
 *
 * @note Instantiates one set of function execution functions for each
 *   concurrent coverage type.
 */
#define INSTANTIATE_FUNCTION_EXECUTION_CALLBACK_FUNCTION(cctype) \
  template VOID PIN_FAST_ANALYSIS_CALL \
  afterStackPtrSetByLongJump< cctype >(THREADID tid, ADDRINT sp); \
  template VOID PIN_FAST_ANALYSIS_CALL \
  beforeFunctionCalled< cctype >(THREADID tid, ADDRINT sp, ADDRINT idx); \
  template VOID PIN_FAST_ANALYSIS_CALL \
  beforeFunctionReturned< cctype >(THREADID tid, ADDRINT sp BFR_ADDITIONAL_PARAMS)

// Instantiate function execution functions
// TODO: use templates instead of macros (like with memory access functions)
INSTANTIATE_FUNCTION_EXECUTION_CALLBACK_FUNCTION(CC_NONE);
INSTANTIATE_FUNCTION_EXECUTION_CALLBACK_FUNCTION(CC_PREDS);

/**
 * Registers a callback function which will be called after a thread creates
 *   a new thread and store information about the thread.
 *
 * @tparam BT A type of backtraces the framework is using.
 *
 * @param tid A thread which is creating a new thread.
 * @param sp A value of the stack pointer register.
 * @param threadAddr An address at which is the new thread stored.
 * @param funcDesc A structure containing the description of the function
 *   creating the thread.
 */
template < BacktraceType BT >
VOID beforeThreadCreate(CBSTACK_FUNC_PARAMS, ADDRINT* threadAddr, VOID* funcDesc)
{
#if defined(TARGET_IA32) || defined(TARGET_LINUX)
  if (BT & BT_LIGHTWEIGHT)
  { // Return address of the thread creation function is now on top of the call
    // stack, but in the after callback we cannot get this info, we get it here
    funcDesc = new std::pair< FunctionDesc*, std::string >(
      static_cast< FunctionDesc* >(funcDesc),
      makeBacktraceLocation< BV_DETAILED, FI_LOCKED >(STACK_VALUE(sp))
    );
  }
#endif

  // Register a callback function to be called after creating a thread
  if (CALL_AFTER(afterThreadCreate< BT >)) return;

  // We can safely assume that the argument is a pointer or reference
  THREAD_DATA->arg = *threadAddr;
}

/**
 * Creates a mapping between a newly created thread and a location where the
 *   thread was created.
 *
 * @tparam BT A type of backtraces the framework is using.
 *
 * @param tid A thread which created the new thread.
 * @param retVal A value returned by the thread creation function.
 * @param data An arbitrary data passed to the function.
 */
template < BacktraceType BT >
VOID afterThreadCreate(THREADID tid, ADDRINT* retVal, VOID* data)
{
  if (BT & BT_PRECISE)
  { // Top location in the backtrace is location where the thread was created
    g_threadCreateLocMap.insert(
      getThread(&THREAD_DATA->arg, static_cast< FunctionDesc* >(data)).q(),
      retrieveCall(THREAD_DATA->backtrace.front())
    );
  }
#if defined(TARGET_IA32) || defined(TARGET_LINUX)
  else if (BT & BT_LIGHTWEIGHT)
  { // No need for a temporary variable, someone save trees, we save memory :)
    #define DATA static_cast< std::pair< FunctionDesc*, std::string >* >(data)

    // We already have the location where the thread was created from before
    g_threadCreateLocMap.insert(
      getThread(&THREAD_DATA->arg, DATA->first).q(),
      DATA->second
    );

    delete DATA;
  }
#endif
}

/**
 * @brief Instantiates a concrete code of a thread create callback function
 *   from a template.
 *
 * @param bttype A type of backtraces the framework is using.
 */
#define INSTANTIATE_THREAD_CREATE_CALLBACK_FUNCTION(bttype) \
  template VOID PIN_FAST_ANALYSIS_CALL \
  beforeThreadCreate< bttype >(CBSTACK_FUNC_PARAMS, ADDRINT* threadAddr, \
    VOID* funcDesc); \
  template VOID PIN_FAST_ANALYSIS_CALL \
    afterThreadCreate< bttype >(THREADID tid, ADDRINT* retVal, VOID* data)

// Instantiate callback functions called before and after thread creation
INSTANTIATE_THREAD_CREATE_CALLBACK_FUNCTION(BT_NONE);
INSTANTIATE_THREAD_CREATE_CALLBACK_FUNCTION(BT_LIGHTWEIGHT);
INSTANTIATE_THREAD_CREATE_CALLBACK_FUNCTION(BT_FULL);
INSTANTIATE_THREAD_CREATE_CALLBACK_FUNCTION(BT_PRECISE);

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

  // Now we can associate the thread with the location where it was created
  THREAD_DATA->tcloc = g_threadCreateLocMap.get(getThread(threadAddr,
    static_cast< FunctionDesc* >(funcDesc)).q());
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
  assert(!THREAD_DATA->ljthread.is_valid());

  // Save the joined thread for the time when the join function if left
  THREAD_DATA->ljthread = thread;

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
  // The joined thread must be the last thread joined with this thread
  THREAD& thread = THREAD_DATA->ljthread;

  // Cannot leave a join function before entering it
  assert(thread.is_valid());

  for (JoinFunPtrVector::iterator it = g_afterJoinVector.begin();
    it != g_afterJoinVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, g_threadIdMap.get(thread.q()));
  }

  // This will tell the asserts that we left the join function
  thread.invalidate();
}

/**
 * Registers a function used to initialise a thread.
 *
 * @note This function is called when a thread is about to start its execution.
 *
 * @param callback A thread initialisation function.
 * @param data An arbitrary data passed to the function when it is called.
 */
VOID addThreadInitFunction(THREADINITFUNPTR callback, VOID* data)
{
  g_threadInitFunctions.push_back(make_pair(callback, data));
}

/**
 * Gets a position of the last location (call) in a backtrace of a thread stored
 *   in the (call) index.
 *
 * @warning If precise backtraces are not used, the behaviour of this function
 *   is undefined!
 *
 * @param tid A number identifying the thread.
 * @return The position of the last location (call) in the backtrace of the
 *   thread stored in the (call) index.
 */
index_t getLastBacktraceLocationIndex(THREADID tid)
{
  return (THREAD_DATA->backtrace.empty()) ? -1 : THREAD_DATA->backtrace.front();
}

/**
 * Gets the last location (call) in a backtrace of a thread.
 *
 * @warning If precise backtraces are not used, the behaviour of this function
 *   is undefined!
 *
 * @param tid A number identifying the thread.
 * @return The last location (call) in the backtrace of the thread.
 */
std::string getLastBacktraceLocation(THREADID tid)
{
  return (THREAD_DATA->backtrace.empty()) ? "<unknown>"
    : retrieveCall(THREAD_DATA->backtrace.front());
}

/**
 * Gets a size of a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The size of a backtrace of a thread.
 */
size_t getBacktraceSize(THREADID tid)
{
  return THREAD_DATA->backtrace.size();
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

/**
 * Gets a location where a thread was created.
 *
 * @param tid A number identifying the thread.
 * @param location A location where the thread was created.
 */
VOID THREAD_GetThreadCreationLocation(THREADID tid, std::string& location)
{
  location = THREAD_DATA->tcloc;
}

/** End of file thread.cpp **/
