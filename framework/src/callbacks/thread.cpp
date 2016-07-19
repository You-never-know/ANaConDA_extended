/**
 * @brief Contains implementation of thread-related callback functions.
 *
 * A file containing implementation of callback functions called when some
 *   thread starts or finishes.
 *
 * @file      thread.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-03
 * @date      Last Update 2016-07-19
 * @version   0.13.11
 */

#include "thread.h"

#include <assert.h>

#include <functional>

#include <boost/foreach.hpp>

#include "shared.hpp"

#include "../anaconda.h"
#include "../cbstack.h"

#include "../monitors/preds.hpp"

#include "../utils/backtrace.hpp"
#include "../utils/rwmap.hpp"
#include "../utils/thread.h"
#include "../utils/tldata.hpp"
#include "../utils/unwind.h"

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

namespace
{ // Internal type definitions and variables (usable only within this module)
  typedef std::vector< ADDRINT > FunctionVector;
  typedef std::vector< ADDRINT > BtSpVector;

  // Types of containers for storing various callback functions
  typedef std::vector< std::pair< THREADINITFUNPTR, VOID* > >
    ThreadInitCallbackContainerType;
  typedef std::vector< THREADFUNPTR > ThreadStartedCallbackContainerType;
  typedef std::vector< THREADFUNPTR > ThreadFinishedCallbackContainerType;
  typedef std::vector< FORKFUNPTR > ThreadForkedCallbackContainerType;
  typedef std::vector< THREADFUNPTR > FunctionEnteredCallbackContainerType;
  typedef std::vector< THREADFUNPTR > FunctionExitedCallbackContainerType;

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
    FunctionVector functions; //!< A list of currently executing functions.
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
   * Gets a container for storing functions which should be called when a
   *   thread is being initialised.
   *
   * @note The contained functions will be called right before the functions
   *   which are called when a thread starts.
   *
   * @note Some global variables (e.g. global variables of the ThreadLocalData
   *   type) register a thread initialisation callback function when they are
   *   constructed (initialised). As these variables might be constructed when
   *   the program starts, we need to ensure that the container for storing the
   *   registered callback functions is already constructed when performing the
   *   registration. Else, we will not be able to store the registered callback
   *   functions we should call later! Defining the container first in a module
   *   will not help, because the registration is performed in various modules
   *   and the initialisation order across modules is not defined (this is the
   *   well-known 'static initialisation order fiasco'). One way to solve this
   *   problem is to use a global function which will construct the container
   *   when it is needed for the first time. This approach always ensures that
   *   the container will be constructed when the registration is performed as
   *   it will construct the container when it is needed for the first time.
   *
   * @return A container for storing functions which should be called when a
   *   thread is being initialised.
   */
  ThreadInitCallbackContainerType& getThreadInitCallbacks()
  {
    // Construct the container for storing the registered callback functions
    // just at the time when we need to access it for the first time
    static ThreadInitCallbackContainerType g_threadInitCallbacks;

    // Return the container for storing the registered callback functions
    return g_threadInitCallbacks;
  }

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
  /**
   * @brief Contains functions which should be called when a thread creates a
   *   new thread (forks into two threads).
   */
  ThreadForkedCallbackContainerType g_threadForkedCallbacks;
  /**
   * @brief Contains functions which should be called when a thread enters a
   *   function (starts execution of a function).
   */
  FunctionEnteredCallbackContainerType g_functionEnteredCallbacks;
  /**
   * @brief Contains functions which should be called when a thread exits a
   *   function (finishes execution of a function).
   */
  FunctionExitedCallbackContainerType g_functionExitedCallbacks;

  ThreadLocalData< ThreadData > g_data; //!< Private data of running threads.

  /**
   * @brief A function which should be called before a thread is created.
   */
  AFUNPTR g_beforeThreadCreateCallback = NULL;
  /**
   * @brief A function for accessing a backtrace of a thread.
   */
  BACKTRACEFUNPTR g_getBacktraceImpl = NULL;
  /**
   * @brief A function for translating backtrace entries to strings describing
   *   these entries (e.g. translating indexes or addresses to locations).
   */
  BACKTRACESYMFUNPTR g_getBacktraceSymbolsImpl = NULL;

  ImmutableRWMap< UINT32, THREADID > g_threadIdMap(0);
  ImmutableRWMap< UINT32, std::string > g_threadCreateLocMap("<unknown>");

  PredecessorsMonitor< FileWriter >* g_predsMon;

  /**
   * @brief A structure used to synchronise threads during thread creation.
   */
  typedef struct ThreadCreationBarrier_s
  {
    /**
     * @brief Used to synchronise the old thread with the newly created one.
     */
    PIN_SEMAPHORE semOld;
    /**
     * @brief Used to synchronise the newly created thread with the old one.
     */
    PIN_SEMAPHORE semNew;

    /**
     * Constructs a new structure used to synchronise threads during their
     *   creation.
     */
    ThreadCreationBarrier_s()
    {
      PIN_SemaphoreInit(&semOld);
      PIN_SemaphoreInit(&semNew);
    }

    /**
     * Destroys a structure used to synchronise threads during their creation.
     */
    ~ThreadCreationBarrier_s()
    {
      PIN_SemaphoreFini(&semOld);
      PIN_SemaphoreFini(&semNew);
    }

    /**
     * Waits for the old thread to prepare data used by the new thread.
     */
    void waitForOld()
    {
      PIN_SemaphoreWait(&semOld);
      PIN_SemaphoreClear(&semOld);
    }

    /**
     * Waits for the new thread to prepare data used by the old thread.
     */
    void waitForNew()
    {
      PIN_SemaphoreWait(&semNew);
      PIN_SemaphoreClear(&semNew);
    }

    /**
     * Signals that the old thread has finished preparing the data used by the
     *   new thread.
     */
    void oldReady()
    {
      PIN_SemaphoreSet(&semOld);
    }

    /**
     * Signals that the new thread has finished preparing the data used by the
     *   old thread.
     */
    void newReady()
    {
      PIN_SemaphoreSet(&semNew);
    }
  } ThreadCreationBarrier;

  /**
   * @brief A list of all barriers currently used to synchronise old and newly
   *   created threads.
   *
   * @note The implementation ensures that we are always refreshing references
   *   to values after they are updated so we never work with invalid pointers
   *   here.
   */
  UnsafeRWMap< UINT32, ThreadCreationBarrier* > g_threadCreationBarrier(NULL);
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
    symbols.push_back(std::string() + *retrieveCall(bt[i]));
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
  BOOST_FOREACH(ThreadInitCallbackContainerType::const_reference fdpair,
    getThreadInitCallbacks())
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
 * Updates the internal call stack to match the call stack of the program being
 *   monitored. Triggers @e function @e exited notifications for all functions
 *   the program is returning from by unwinding their portion of the call stack.
 *
 * When unwinding the stack, the program is reverting its execution to a prior
 *   state where it was executing some other function from its call stack. This
 *   function called all of the functions from which we are returning now (and
 *   whose portions of the stack are being unwinded). We are thus removing all
 *   functions up to this function from the call stack, triggering @e function
 *   @e exited notifications for each of them.
 *
 * @note This function is called immediately after an unwind function finishes
 *   unwinding the stack. Actually, it is called right after an instruction in
 *   the unwind function sets the new value of the stack pointer.
 *
 * @tparam Compare A comparison object used to determine from how many function
 *   we are returning.
 *
 * @param tid A number identifying the thread whose stack is being unwinded.
 * @param sp A new value of the stack pointer register of the thread.
 */
template < class Compare = std::less< ADDRINT > >
VOID PIN_FAST_ANALYSIS_CALL afterUnwind(THREADID tid, ADDRINT sp)
{
  // Call all registered callback functions for each of the exiting functions
  cbstack::afterUnwind(tid, sp);

  // TODO: This should be also checked in the while loop
  if (g_data.get(tid)->btsplist.empty()) return;

  // As we are monitoring function calls, we are not returning to the function
  // to which is the long jump jumping, but to the call to this function. This
  // means that if the stored SP is equal to the SP where the long jump is
  // jumping, it is the call from the function to which we are jumping and we
  // need to delete this call from the backtrace too
  while (Compare()(g_data.get(tid)->btsplist.back(), sp))
  { // Backtrack to the call which executed the function where we are jumping
    g_data.get(tid)->backtrace.pop_front();
    g_data.get(tid)->btsplist.pop_back();

    BOOST_FOREACH(FunctionExitedCallbackContainerType::const_reference callback,
      g_functionExitedCallbacks)
    { // Call all callback functions registered by the user (used analyser)
      callback(tid);
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
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 * @param idx An index of the function which the thread is calling.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionCalled(THREADID tid, ADDRINT sp,
  ADDRINT idx)
{
#if ANACONDA_DEBUG_CALL_TRACKING == 1
  CONSOLE("Thread " + decstr(tid) + ": beforeFunctionCalled: sp="
    + hexstr(sp) + ", call=" + *retrieveCall(idx)
    + " [call stack size is "
    + decstr(g_data.get(tid)->backtrace.size()) + "]\n");
#endif
  if (!g_data.get(tid)->btsplist.empty())
    if (g_data.get(tid)->btsplist.back() < sp)
      CONSOLE("WARNING: Previous value of SP [" + hexstr(
        (BtSpVector::value_type)g_data.get(tid)->btsplist.back())
        + "] is lower than the current value of SP [" + hexstr(sp) + "]\n");

  // Add the call to be executed to the backtrace
  g_data.get(tid)->backtrace.push_front(idx);
  g_data.get(tid)->btsplist.push_back(sp - sizeof(ADDRINT));
}

/**
 * Notifies all listeners that a thread finished the execution of a function.
 *   Updates the call stack of the thread by removing the information about
 *   the function that finished its execution.
 *
 * @param tid A thread in which the function finished its execution.
 * @param retVal A return value of the function.
 * @param data Arbitrary data passed to the callback function (not used).
 */
VOID afterFunctionExecuted(THREADID tid, ADDRINT* retVal, VOID* data)
{
  BOOST_FOREACH(FunctionExitedCallbackContainerType::const_reference callback,
    g_functionExitedCallbacks)
  { // Call all callback functions registered by the user (used analyser)
    callback(tid);
  }

  // Return to the function from which the current function was executed
  g_data.get(tid)->functions.pop_back();
}

/**
 * Notifies all listeners that a thread is about to execute a function. Updates
 *   the call stack of the thread with the information about the function to be
 *   executed.
 *
 * @note This function is called immediately before a thread executes the first
 *   instruction of a function.
 *
 * @param tid A number identifying the thread executing the function.
 * @param sp A value of the stack pointer register of the thread.
 * @param idx A position of the function in the function index.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionExecuted(THREADID tid, ADDRINT sp,
  ADDRINT idx)
{
#if ANACONDA_DEBUG_FUNCTION_TRACKING == 1
  CONSOLE("Thread " + decstr(tid) + ": beforeFunctionExecuted: sp="
    + hexstr(sp) + ", function=" + *retrieveFunction(idx)
    + " [function stack size is "
    + decstr(g_data.get(tid)->functions.size()) + "]\n");

  if (g_data.get(tid)->btsplist.empty())
    CONSOLE("WARNING: beforeFunctionExecuted: no call before function\n");
  else if (g_data.get(tid)->btsplist.back() != sp)
    CONSOLE("WARNING: beforeFunctionExecuted: SP of call "
      + hexstr(g_data.get(tid)->btsplist.back())
      + " != SP of function " + hexstr(sp) + "\n");
#endif
  // If we fail to register the callback function, it means we are re-executing
  // the function without calling it and thus we should ignore this situation
  if (REGISTER_AFTER_CALLBACK(afterFunctionExecuted, NULL)) return;

  // Add the function to be executed to the list of functions
  g_data.get(tid)->functions.push_back(idx);

  BOOST_FOREACH(FunctionEnteredCallbackContainerType::const_reference callback,
    g_functionEnteredCallbacks)
  { // Call all callback functions registered by the user (used analyser)
    callback(tid);
  }
}

/**
 * Updates a backtrace of a thread. Removes information about the function from
 *   which is the thread returning.
 *
 * @note This function is called immediately before a \c RETURN instruction is
 *   executed.
 *
 * @param tid A number identifying the thread.
 * @param sp A value of the stack pointer register of the thread.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionReturned(THREADID tid, ADDRINT sp,
  ADDRINT idx)
{
#if ANACONDA_DEBUG_CALL_TRACKING == 1
  CONSOLE("Thread " + decstr(tid) + ": beforeFunctionReturned: sp="
    + hexstr(sp) + ", instruction=" + *retrieveInstruction(idx)
    + " [call stack size is "
    + decstr(g_data.get(tid)->backtrace.size()) + "]\n");
#endif
  // We can't have more returns than calls
  assert(!g_data.get(tid)->backtrace.empty());

  if (g_data.get(tid)->btsplist.back() != sp)
  { // We are not returning from the last function we called
    CONSOLE("WARNING: (SP of call) " + hexstr(g_data.get(tid)->btsplist.back())
      + " != " + hexstr(sp) + " (SP of return)!\n");

    return; // Ignore this return
  }

  // Return to the call which executed the function where we are returning
  g_data.get(tid)->backtrace.pop_front();
  g_data.get(tid)->btsplist.pop_back();
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
  // We do not know the ID PIN assigned to the newly created thread, however,
  // we can get an abstraction of the concrete thread representation used by
  // the multi-threaded library which the monitored program is using
  THREAD thread = mapArgTo< THREAD >(&g_data.get(tid)->arg,
    static_cast< HookInfo* >(data));

  // Helper variables
  ThreadCreationBarrier* barrier;

  while ((barrier = g_threadCreationBarrier.get(thread.q())) == NULL)
  { // Wait until the newly created thread gives us a barrier to synchronise
    // with it, usually the barrier would be already there when we get here
    // so the sleeping here will be very short (or does not occur at all)
    PIN_Sleep(1);
  }

  // Wait for the newly created thread to determine the ID PIN assigned to it
  barrier->waitForNew();

  if (BT & BT_PRECISE)
  { // Top location in the backtrace is location where the thread was created
    g_threadCreateLocMap.insert(
      mapArgTo< THREAD >(&g_data.get(tid)->arg,
        static_cast< HookInfo* >(data)).q(),
      std::string() + *retrieveCall(g_data.get(tid)->backtrace.front())
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

  // We registered the location where the thread was started (created)
  barrier->oldReady();

  // Wait for the newly created thread to finish its initialisation
  barrier->waitForNew();

  BOOST_FOREACH(ThreadForkedCallbackContainerType::const_reference callback,
    g_threadForkedCallbacks)
  { // Call all callback functions registered by the user (used analyser)
    callback(tid, getThreadId(thread));
  }

  // We notified all analysers that a new thread was created (forked)
  barrier->oldReady();
}

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
  // Get an abstraction of the concrete thread representation used by the
  // multi-threaded library, this information is know to the thread which
  // created this new thread (the only information know by both threads)
  THREAD thread = mapArgTo< THREAD >(arg, hi);

  // Create a mapping between the thread abstraction and ID given by PIN
  g_threadIdMap.insert(thread.q(), tid);

  // We will use this object to synchronise with the thread that created us
  ThreadCreationBarrier* barrier = new ThreadCreationBarrier();

  // Publish this object to all threads, only the one which created us will
  // access it as only this thread knows thread abstraction representing us
  g_threadCreationBarrier.update(thread.q(), barrier);

  barrier->newReady(); // We published our thread ID
  barrier->waitForOld(); // We need to wait for our thread creation location

  // Now we can associate the thread with the location where it was created
  g_data.get(tid)->tcloc = g_threadCreateLocMap.get(thread.q());

  // The other thread already has a pointer to the barrier object so it is
  // safe to reset it to NULL, if the thread object is reused, it will not
  // find the pointer to the old (deleted) barrier object in the map now
  g_threadCreationBarrier.update(thread.q(), NULL);

  barrier->newReady(); // We finished our initialisation
  barrier->waitForOld(); // Wait until the other thread notifies analysers

  delete barrier; // We do not need the barrier anymore
}

/**
 * Setups the thread execution monitoring, i.e., setups the functions which will
 *   be used for instrumenting the thread-execution-related functions etc.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupThreadModule(Settings* settings)
{
  // Helper macros used only in this function
  #define OPTION(name) settings->get< std::string >(name)

  if (OPTION("backtrace.type") == "precise")
  { // Precise: create backtraces on the fly by monitoring calls and returns
    g_beforeThreadCreateCallback = (AFUNPTR)beforeThreadCreate< BT_PRECISE >;

    g_getBacktraceImpl = getPreciseBacktrace;
    g_getBacktraceSymbolsImpl = getPreciseBacktraceSymbols;
  }
  else if (OPTION("backtrace.type") == "full")
  { // Full: create backtraces on the fly by monitoring execution of functions
    g_beforeThreadCreateCallback = (AFUNPTR)beforeThreadCreate< BT_FULL >;

    // TODO: add support for this type of backtraces
  }
  else if (OPTION("backtrace.type") == "lightweight")
  { // Lightweight: create backtraces on demand by walking the stack
    g_beforeThreadCreateCallback = (AFUNPTR)beforeThreadCreate< BT_LIGHTWEIGHT >;

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
  { // None: no backtraces will be created
    g_beforeThreadCreateCallback = (AFUNPTR)beforeThreadCreate< BT_NONE >;

    g_getBacktraceImpl = [] (THREADID tid, Backtrace& bt) {};
    g_getBacktraceSymbolsImpl = [] (Backtrace& bt, Symbols& symbols) {};
  }

  BOOST_FOREACH(HookInfo* hi, settings->getHooks())
  { // Setup the functions able to instrument the thread operations
    switch (hi->type)
    { // Configure only thread-related hooks, ignore the others
      case HT_THREAD_CREATE: // A thread creation operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)g_beforeThreadCreateCallback,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->thread - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_THREAD_INIT: // A thread initialisation operation
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeThreadInit,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->thread - 1,
            IARG_PTR, hi,
            IARG_END);
        };
        break;
      case HT_UNWIND: // A function unwinding thread's stack
        hi->instrument = [] (RTN rtn, HookInfo* hi) {
          switch (hi->cbtype)
          { // Each unwind function may require a different callback function
            case UNWIND_NO_RET: // Unwind function without return
              instrumentUnwindFunction(rtn,
                (UNWINDFUNPTR)afterUnwind< std::less_equal< ADDRINT > >);
              break;
            case UNWIND_RETURN: // Unwind function with return
              instrumentUnwindFunction(rtn,
                (UNWINDFUNPTR)afterUnwind< std::less< ADDRINT > >);
              break;
            default: // Should not reach this code
              assert(false);
              break;
          }
        };
        break;
      default: // Ignore other hooks
        break;
    }
  }

  g_predsMon = &settings->getCoverageMonitors().preds;
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
  getThreadInitCallbacks().push_back(make_pair(callback, data));
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
    : retrieveLocation(retrieveCall(g_data.get(tid)->backtrace.front())->location)->file;
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
 * Registers a callback function which will be called when a thread creates
 *   a new thread (forks into two threads).
 *
 * @note This callback function is called @em after the new thread is fully
 *   initialised, i.e., after native initialisation functions have finished
 *   their execution. This means that the callback functions executed when
 *   a thread has started (registered via @c THREAD_ThreadStarted function)
 *   are executed before the callback functions registered here.
 *
 * @param callback A callback function which should be called when a thread
 *   creates a new thread (forks into two threads).
 */
VOID THREAD_ThreadForked(FORKFUNPTR callback)
{
  g_threadForkedCallbacks.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread enters a
 *   function (starts execution of a function).
 *
 * @param callback A callback function which should be called when a thread
 *   enters a function.
 */
VOID THREAD_FunctionEntered(THREADFUNPTR callback)
{
  g_functionEnteredCallbacks.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread exits a
 *   function (finishes execution of a function).
 *
 * @param callback A callback function which should be called when a thread
 *   exits a function.
 */
VOID THREAD_FunctionExited(THREADFUNPTR callback)
{
  g_functionExitedCallbacks.push_back(callback);
}

/**
 * Registers a callback function which will be called when a thread executes a
 *   specific function (starts execution of a function). The callback function
 *   can access one of the arguments given to the executed function.
 *
 * @param name A name of the function.
 * @param callback A callback function which should be called when a thread
 *   executes a function.
 * @param arg A position of the argument the callback function should access.
 *   The first argument of the function has a position @c 1.
 */
VOID THREAD_FunctionExecuted(const char* name, ARG1FUNPTR callback, UINT32 arg)
{
  // Create a new hook for the function to be monitored
  HookInfo* hi = new HookInfo(HT_DATA_FUNCTION, arg);

  // Use the custom data to store the address of the callback function
  hi->data = (void*)callback;

  // Define how to instrument the function (data=callback, idx=argument)
  hi->instrument = [] (RTN rtn, HookInfo* hi) {
    RTN_InsertCall(
      rtn, IPOINT_BEFORE, (AFUNPTR)hi->data,
      IARG_THREAD_ID,
      IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->idx - 1,
      IARG_END);
  };

  // Register the new hook so the framework starts monitoring it
  Settings::Get()->registerHook(name, hi);
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

/**
 * Gets a function whose code is currently being executed in a specific thread.
 *
 * @param tid A number identifying the thread executing the function.
 * @param function A backtrace location describing the function.
 */
VOID THREAD_GetCurrentFunction(THREADID tid, std::string& function)
{
  function = (g_data.get(tid)->functions.empty())
    ? "" : retrieveFunction(g_data.get(tid)->functions.back())->name;
}

/**
 * Gets a number identifying the currently executed thread.
 *
 * @warning This ID may be reused by new threads after this thread finishes its
 *   execution.
 *
 * @return A number identifying the currently executed thread.
 */
THREADID THREAD_GetThreadId()
{
  return PIN_ThreadId();
}

/**
 * Gets a number uniquely identifying the currently executed thread.
 *
 * @note This ID is never assigned to other threads.
 *
 * @return A number uniquely identifying the currently executed thread.
 */
PIN_THREAD_UID THREAD_GetThreadUid()
{
  return PIN_ThreadUid();
}

/** End of file thread.cpp **/
