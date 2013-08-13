/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2013-08-13
 * @version   0.11.5
 */

#include "thread.h"

#include <assert.h>

#include <boost/foreach.hpp>

#include "shared.hpp"

#include "../anaconda.h"

#include "../monitors/preds.hpp"

#include "../utils/backtrace.hpp"
#include "../utils/rwmap.hpp"
#include "../utils/thread.h"

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
  REGISTER_AFTER_CALLBACK(callback, static_cast< VOID* >(hi))

// Declarations of static functions (usable only within this module)
template< BacktraceType BT >
static VOID afterThreadCreate(THREADID tid, ADDRINT* retVal, VOID* data);

namespace
{ // Internal type definitions and variables (usable only within this module)
  typedef std::vector< ADDRINT > BtSpVector;

  // Types of containers for storing various callback functions
  typedef std::vector< std::pair< THREADINITFUNPTR, VOID* > >
    ThreadInitCallbackContainerType;
  typedef std::vector< THREADFUNPTR > ThreadStartedCallbackContainerType;
  typedef std::vector< THREADFUNPTR > ThreadFinishedCallbackContainerType;

  // Types of functions for retrieving backtrace information
  typedef VOID (*BACKTRACEFUNPTR)(THREADID tid, Backtrace& bt);
  typedef VOID (*BACKTRACESYMFUNPTR)(Backtrace& bt, Symbols& symbols);

  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    ADDRINT bp; //!< A value of the thread's base pointer register.
    Backtrace backtrace; //!< The current backtrace of a thread.
    BtSpVector btsplist; //!< The values of stack pointer of calls in backtrace.
    std::string ltcloc; //!< A location where the last thread was created.
    std::string tcloc; //!< A location where a thread was created.
    ADDRINT arg; //!< A value of an argument of a function called by a thread.

    /**
     * Constructs a ThreadData_s object.
     */
    ThreadData_s() : bp(0), backtrace(), btsplist(), tcloc("<unknown>"), arg(0)
      {}
  } ThreadData;

  /**
   * @brief Contains functions which should be called when a thread is being
   *   initialised.
   *
   * @note These functions will be called right before the functions which are
   *   called when a thread starts.
   *
   * @warning Some global variables (e.g. ThreadLocalData global variables)
   *   register a thread initialisation callback function when constructed.
   *   Because both these variables and this container are constructed when
   *   the programs starts, it is important that this container is created
   *   (and initialised) before the global variables are. Else some of the
   *   thread initialisation callback functions might not be registered
   *   properly!
   */
  ThreadInitCallbackContainerType g_threadInitCallbacks;
  /**
   * @brief Contains functions which should be called when a thread starts its
   *   execution.
   *
   * @note These functions will be called right after the functions which are
   *   called when a thread is being initialised.
   */
  ThreadStartedCallbackContainerType g_threadStartedCallbacks;
  /**
   * @brief Contains functions which should be called when a thread finishes its
   *   execution.
   */
  ThreadFinishedCallbackContainerType g_threadFinishedCallbacks;

  ThreadLocalData< ThreadData > g_data; //!< Private data of running threads.

  /**
   * @brief A function for accessing a backtrace of a thread.
   */
  BACKTRACEFUNPTR g_getBacktraceImpl = NULL;
  /**
   * @brief A function for translating backtrace entries to strings describing
   *   these entries (e.g. translating indexes or addresses to locations).
   */
  BACKTRACESYMFUNPTR g_getBacktraceSymbolsImpl = NULL;

  RWMap< UINT32, THREADID > g_threadIdMap(0);
  RWMap< UINT32, std::string > g_threadCreateLocMap("<unknown>");

  PredecessorsMonitor< FileWriter >* g_predsMon;
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
  ADDRINT bp = g_data.get(tid)->bp;

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
  bt = g_data.get(tid)->backtrace;
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
    g_getBacktraceImpl = getLightweightBacktrace;

    if (settings->get< std::string >("backtrace.verbosity") == "minimal")
    { // Minimal: locations only
      g_getBacktraceSymbolsImpl = getLightweightBacktraceSymbols< BV_MINIMAL >;
    }
    else
    { // Detailed: names of images and functions + locations
      g_getBacktraceSymbolsImpl = getLightweightBacktraceSymbols< BV_DETAILED >;
    }
  }
  else
  { // Precise: create backtraces on the fly by monitoring calls and returns
    g_getBacktraceImpl = getPreciseBacktrace;
    g_getBacktraceSymbolsImpl = getPreciseBacktraceSymbols;
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
  BOOST_FOREACH(ThreadInitCallbackContainerType::const_reference fdpair,
    g_threadInitCallbacks)
  { // Call all thread initialisation functions, stored as (func, data) pairs
    fdpair.first(tid, fdpair.second);
  }

  BOOST_FOREACH(ThreadStartedCallbackContainerType::const_reference callback,
    g_threadStartedCallbacks)
  { // Call all callback functions registered by the user (used analyser)
    callback(tid);
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
  BOOST_FOREACH(ThreadFinishedCallbackContainerType::const_reference callback,
    g_threadFinishedCallbacks)
  { // Call all callback functions registered by the user (used analyser)
    callback(tid);
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
  g_data.get(tid)->bp = sp;
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
  g_data.get(tid)->bp = (STACK_VALUE(sp) > sp) ? STACK_VALUE(sp) : 0;
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
  while (g_data.get(tid)->btsplist.back() <= sp)
  { // Backtrack to the call which executed the function where we are jumping
    g_data.get(tid)->backtrace.pop_front();
    g_data.get(tid)->btsplist.pop_back();

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
  if (!g_data.get(tid)->btsplist.empty())
    if (g_data.get(tid)->btsplist.back() < sp)
      WARNING("Previous value of SP [" + hexstr(
        (BtSpVector::value_type)g_data.get(tid)->btsplist.back())
        + "] is lower than the current value of SP [" + hexstr(sp) + "]\n");

  // Add the call to be executed to the backtrace
  g_data.get(tid)->backtrace.push_front(idx);
  g_data.get(tid)->btsplist.push_back(sp);

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
  assert(!g_data.get(tid)->backtrace.empty());

  // Return to the call which executed the function where we are returning
  g_data.get(tid)->backtrace.pop_front();
  g_data.get(tid)->btsplist.pop_back();

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
 * @param tid A thread which is about to create a new thread.
 * @param sp A value of the stack pointer register.
 * @param arg A pointer to the argument representing the thread which is about
 *   to be created.
 * @param hi A structure containing information about a function creating the
 *   thread.
 */
template< BacktraceType BT >
VOID beforeThreadCreate(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
#if defined(TARGET_IA32) || defined(TARGET_LINUX)
  if (BT & BT_LIGHTWEIGHT)
  { // Return address of the thread creation function is now on top of the call
    // stack, but in the after callback we cannot get this info, we get it here
    g_data.get(tid)->ltcloc = makeBacktraceLocation< BV_DETAILED, FI_LOCKED >(
      STACK_VALUE(sp));
  }
#endif

  // Register a callback function to be called after creating a thread
  if (CALL_AFTER(afterThreadCreate< BT >)) return;

  // We can safely assume that the argument is a pointer or reference
  g_data.get(tid)->arg = *arg;
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
template< BacktraceType BT >
VOID afterThreadCreate(THREADID tid, ADDRINT* retVal, VOID* data)
{
  if (BT & BT_PRECISE)
  { // Top location in the backtrace is location where the thread was created
    g_threadCreateLocMap.insert(
      mapArgTo< THREAD >(&g_data.get(tid)->arg,
        static_cast< HookInfo* >(data)).q(),
      retrieveCall(g_data.get(tid)->backtrace.front())
    );
  }
#if defined(TARGET_IA32) || defined(TARGET_LINUX)
  else if (BT & BT_LIGHTWEIGHT)
  {  // We already have the location where the thread was created from before
    g_threadCreateLocMap.insert(
      mapArgTo< THREAD >(&g_data.get(tid)->arg,
        static_cast< HookInfo* >(data)).q(),
      g_data.get(tid)->ltcloc
    );
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
  beforeThreadCreate< bttype >(CBSTACK_FUNC_PARAMS, ADDRINT* arg, \
    HookInfo* hi); \
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
 * @param tid A thread which is about to be initialised.
 * @param sp A value of the stack pointer register.
 * @param arg A pointer to the argument representing the thread which is about
 *   to be initialised.
 * @param hi A structure containing information about a function working with
 *   the thread.
 */
VOID beforeThreadInit(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi)
{
  g_threadIdMap.insert(mapArgTo< THREAD >(arg, hi).q(), tid);

  // Now we can associate the thread with the location where it was created
  g_data.get(tid)->tcloc = g_threadCreateLocMap.get(
    mapArgTo< THREAD >(arg, hi).q());
}

/**
 * Setups the thread execution monitoring, i.e., setups the functions which will
 *   be used for instrumenting the thread-execution-related functions etc.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupThreadModule(Settings* settings)
{
  //
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
  g_threadInitCallbacks.push_back(make_pair(callback, data));
}

/**
 * Gets a number identifying a thread.
 *
 * @param thread An object representing the thread.
 * @return A number identifying the thread.
 */
THREADID getThreadId(THREAD thread)
{
  return g_threadIdMap.get(thread.q());
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
  return (g_data.get(tid)->backtrace.empty()) ? -1
    : g_data.get(tid)->backtrace.front();
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
  return (g_data.get(tid)->backtrace.empty()) ? "<unknown>"
    : retrieveCall(g_data.get(tid)->backtrace.front());
}

/**
 * Gets a size of a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @return The size of a backtrace of a thread.
 */
size_t getBacktraceSize(THREADID tid)
{
  return g_data.get(tid)->backtrace.size();
}

/**
 * Registers a callback function which will be called when a thread starts.
 *
 * @param callback A callback function which should be called when a thread
 *   starts.
 */
VOID THREAD_ThreadStarted(THREADFUNPTR callback)
{
  g_threadStartedCallbacks.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread finishes.
 *
 * @param callback A callback function which should be called when a thread
 *   finishes.
 */
VOID THREAD_ThreadFinished(THREADFUNPTR callback)
{
  g_threadFinishedCallbacks.push_back(callback);
}

/**
 * Gets a backtrace of a thread.
 *
 * @param tid A number identifying the thread.
 * @param bt A backtrace.
 */
VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt)
{
  g_getBacktraceImpl(tid, bt);
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
  g_getBacktraceSymbolsImpl(bt, symbols);
}

/**
 * Gets a location where a thread was created.
 *
 * @param tid A number identifying the thread.
 * @param location A location where the thread was created.
 */
VOID THREAD_GetThreadCreationLocation(THREADID tid, std::string& location)
{
  location = g_data.get(tid)->tcloc;
}

/** End of file thread.cpp **/
