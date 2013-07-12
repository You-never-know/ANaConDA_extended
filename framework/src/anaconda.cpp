/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2013-07-12
 * @version   0.12.2.1
 */

#include <assert.h>

#include "pin.H"

#include "pin_die.h"

#include "cbstack.h"
#include "config.h"
#include "index.h"
#include "mapper.h"
#include "settings.h"

#include "callbacks/access.h"
#include "callbacks/exception.h"
#include "callbacks/noise.h"
#include "callbacks/sync.h"
#include "callbacks/thread.h"
#include "callbacks/tm.h"

#include "monitors/preds.hpp"

#include "utils/backtrace.hpp"
#include "utils/random.hpp"
#include "utils/seq.hpp"

// Macro definitions
#define INSERT_CALL(callback) \
  RTN_InsertCall( \
    rtn, IPOINT_BEFORE, (AFUNPTR)callback, \
    CBSTACK_IARG_PARAMS, \
    IARG_FUNCARG_ENTRYPOINT_REFERENCE, hi->lock - 1, \
    IARG_PTR, hi, \
    IARG_END)
#define INSERT_CALL_NO_FUNCARGS(callback) \
  RTN_InsertCall( \
    rtn, IPOINT_BEFORE, (AFUNPTR)callback, \
    CBSTACK_IARG_PARAMS, \
    IARG_PTR, hi, \
    IARG_END)

// Type definitions
typedef VOID (*INSERTCALLFUNPTR)(INS ins, IPOINT ipoint, AFUNPTR funptr, ...);

// Helper macros defining parameters needed by memory access callback functions
#define BEFORE_STD_MEMORY_ACCESS_IARG_PARAMS \
  IARG_THREAD_ID, \
  IARG_MEMORYOP_EA, memOpIdx, \
  IARG_UINT32, INS_MemoryOperandSize(ins, memOpIdx), \
  IARG_UINT32, memOpIdx, \
  IARG_ADDRINT, RTN_Address(INS_Rtn(ins)), \
  IARG_ADDRINT, INS_Address(ins), \
  IARG_CONST_CONTEXT
#define AFTER_STD_MEMORY_ACCESS_IARG_PARAMS \
  IARG_THREAD_ID, \
  IARG_UINT32, memOpIdx
#define BEFORE_REP_MEMORY_ACCESS_IARG_PARAMS \
  BEFORE_STD_MEMORY_ACCESS_IARG_PARAMS, \
  IARG_EXECUTING
#define AFTER_REP_MEMORY_ACCESS_IARG_PARAMS \
  AFTER_STD_MEMORY_ACCESS_IARG_PARAMS

// Helper macros for instantiating memory accesses instrumentation code
#define MAIS_BEFORE_STD_CALLBACK beforeCallback
#define MAIS_AFTER_STD_CALLBACK afterCallback
#define MAIS_BEFORE_REP_CALLBACK beforeRepCallback
#define MAIS_AFTER_REP_CALLBACK afterRepCallback

#define INSERT_CALL_STD insertCall
#define INSERT_CALL_REP INS_InsertCall

// Helper macros for instrumenting memory accesses
#define INSTRUMENT_MEMORY_ACCESS(where, type) \
  if (access->MAIS_##where##_##type##_CALLBACK != NULL) \
    INSERT_CALL_##type( \
      ins, IPOINT_##where, access->MAIS_##where##_##type##_CALLBACK, \
      IARG_FAST_ANALYSIS_CALL, \
      where##_##type##_MEMORY_ACCESS_IARG_PARAMS, \
      IARG_END)

namespace
{ // Static global variables (usable only within this module)
  AFUNPTR g_beforeFunctionCalled;
  AFUNPTR g_beforeFunctionReturned;
  AFUNPTR g_afterStackPtrSetByLongJump;

  PredecessorsMonitor< FileWriter >* g_predsMon;
}

/**
 * Prints information about a function which will be executed by a thread.
 *
 * @note This function is called immediately before the thread executes the
 *   first instruction of the function.
 *
 * @param tid A number identifying the thread.
 * @param idx An index of the function which the thread is executing.
 */
VOID PIN_FAST_ANALYSIS_CALL beforeFunctionExecuted(THREADID tid, ADDRINT idx)
{
  CONSOLE("Thread " + decstr(tid) + " is about to execute function "
    + retrieveFunction(idx) + "\n");
}

/**
 * Instruments an instruction if it operates (creates or clears) a stack frame.
 *
 * @param ins An instruction.
 */
inline
VOID instrumentStackFrameOperation(INS ins)
{
  switch (INS_Opcode(ins))
  { // Check if the instruction operates a stack frame
    case XED_ICLASS_PUSH: // A new stack frame might be created
      if (INS_RegRContain(ins, REG_GBP))
      { // Stack pointer contains value of the new base pointer
        INS_InsertCall(
          ins, IPOINT_AFTER, (AFUNPTR)afterBasePtrPushed,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_REG_VALUE, REG_STACK_PTR,
          IARG_END);
      }
      break;
    case XED_ICLASS_POP: // The current stack frame might be cleared
      if (INS_RegWContain(ins, REG_GBP))
      { // Previous base pointer on the top of the stack
        INS_InsertCall(
          ins, IPOINT_BEFORE, (AFUNPTR)beforeBasePtrPoped,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_REG_VALUE, REG_STACK_PTR,
          IARG_END);
      }
      break;
    case XED_ICLASS_LEAVE:
      // Previous base pointer at address given by the current base pointer
      INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)beforeBasePtrPoped,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_GBP,
        IARG_END);
      break;
  }
}

/**
 * Instruments an instruction if the instruction modifies the call stack.
 *
 * @tparam BV Determines the amount of information available in backtraces.
 *
 * @param ins An object representing the instruction.
 * @param data A pointer to arbitrary data.
 */
template < BacktraceVerbosity BV >
VOID instrumentCallStackOperation(INS ins, VOID* data)
{
  switch (INS_Opcode(ins))
  { // Check if the instruction modifies the call stack
    case XED_ICLASS_CALL_FAR:
    case XED_ICLASS_CALL_NEAR:
      INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)g_beforeFunctionCalled,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 0
        IARG_ADDRINT, indexCall(makeBacktraceLocation< BV >(ins)),
#else
        IARG_ADDRINT, indexCall(makeBacktraceLocation< BV_MAXIMAL >(ins)),
#endif
        IARG_END);
      break;
    case XED_ICLASS_RET_FAR:
    case XED_ICLASS_RET_NEAR:
      INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)g_beforeFunctionReturned,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
#if ANACONDA_PRINT_BACKTRACE_CONSTRUCTION == 1
        IARG_ADDRINT, indexFunction(makeBacktraceLocation< BV_MAXIMAL >(ins)),
#endif
        IARG_END);
      break;
  }
}

/**
 * Instruments all memory accesses (reads and writes) of an instruction.
 *
 * @param ins An instruction whose memory accesses should be instrumented.
 * @param mais An object containing memory access instrumentation settings.
 */
inline
VOID instrumentMemoryAccess(INS ins, MemoryAccessInstrumentationSettings& mais)
{
  // Get the number of memory accesses (reads/writes) done by the instruction
  UINT32 memOpCount = INS_MemoryOperandCount(ins);

  // No memory accesses to instrument
  if (memOpCount == 0) return;

  // No Intel instruction have more that 2 memory accesses (at least right now)
  assert(memOpCount <= 2);

  if (INS_IsRet(ins))
  { // Do not instrument returns, they just read from stack (see ISDM-2B 4-469)
    assert(memOpCount == 1);
    return;
  }

  if (INS_IsCall(ins))
  { // Do not instrument calls, they just write to stack and optionally read
    // from a memory (indirect calls) (see ISDM-2A 3-112)
    assert(memOpCount == 1 || memOpCount == 2);
    return;
  }

  if (INS_Opcode(ins) == XED_ICLASS_JMP)
  { // Do not instrument jumps reading the target address from a memory, they
    // are not fall-through and read from read-only parts (see ISDM-2A 3-556)
    assert(memOpCount == 1 && INS_MemoryOperandIsRead(ins, 0));
    return;
  }

  // Just to be sure that we will be able to insert the after calls
  assert(INS_HasFallThrough(ins));

  // Helper variables (better than having 4 nearly same blocks of code)
  INSERTCALLFUNPTR insertCall = INS_InsertCall;
  InstrumentationSettings* access = NULL;

  // Predicated instruction might not be executed at all
  if (INS_IsPredicated(ins)) insertCall = INS_InsertPredicatedCall;

  for (UINT32 memOpIdx = 0; memOpIdx < memOpCount; memOpIdx++)
  { // Instrument all memory accesses (reads and writes)
    if (INS_MemoryOperandIsWritten(ins, memOpIdx))
    { // The memOpIdx-th memory access is a write or update access
      access = (INS_MemoryOperandIsRead(ins, memOpIdx))
        ? &mais.updates : &mais.writes;
    }
    else
    { // The memOpIdx-th memory access is a read access
      access = &mais.reads;
    }

    if (INS_HasRealRep(ins))
    { // Do not use predicated calls for REP instructions (they seems broken)
      INSTRUMENT_MEMORY_ACCESS(BEFORE, REP);
      INSTRUMENT_MEMORY_ACCESS(AFTER, REP);
    }
    else
    { // Use predicated calls for conditional instructions, normal for others
      INSTRUMENT_MEMORY_ACCESS(BEFORE, STD);
      INSTRUMENT_MEMORY_ACCESS(AFTER, STD);
    }

    if (std::count(access->noise->filters.begin(), access->noise->filters.end(),
      NF_PREDECESSORS))
    { // Do not insert noise before accesses which do not have a predecessor
      if (!g_predsMon->hasPredecessor(INS_Address(ins))) continue;
    }

    if (access->noise->filter != NULL)
    { // Some filters active, let them determine if noise should be injected
      insertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)access->noise->filter,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_MEMORYOP_EA, memOpIdx,
        IARG_UINT32, INS_MemoryOperandSize(ins, memOpIdx),
        IARG_ADDRINT, RTN_Address(INS_Rtn(ins)),
        IARG_ADDRINT, INS_Address(ins),
        IARG_CONST_CONTEXT,
        IARG_PTR, access->noise,
        IARG_END);
    }
    else
    { // No filter active, place the noise before every access
      insertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)access->noise->generator,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_UINT32, access->noise->frequency,
        IARG_UINT32, access->noise->strength,
        IARG_END);
    }
  }
}

/**
 * Inserts a monitoring code before a hook (monitored function).
 *
 * @tparam BT A type of backtraces the framework should provide.
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param rtn An object representing the monitored function (hook).
 * @param hi A structure containing information about the hook.
 */
template < BacktraceType BT, ConcurrentCoverage CC >
inline
VOID instrumentHook(RTN rtn, HookInfo* hi)
{
  switch (hi->type)
  { // Instrument the function based on its type
    case HT_LOCK: // A lock function
      INSERT_CALL(beforeLockAcquire< CC >);
      break;
    case HT_UNLOCK: // An unlock function
      INSERT_CALL(beforeLockRelease< CC >);
      break;
    case HT_SIGNAL: // A signal function
      INSERT_CALL(beforeSignal);
      break;
    case HT_WAIT: // A wait function
      INSERT_CALL(beforeWait);
      break;
    case HT_LOCK_INIT: // A lock initialisation function
      INSERT_CALL_NO_FUNCARGS(beforeLockCreate);
      break;
    case HT_GENERIC_WAIT: // A generic wait function
      INSERT_CALL(beforeGenericWait< CC >);
      break;
    case HT_THREAD_CREATE: // A thread creation function
      INSERT_CALL(beforeThreadCreate< BT >);
      break;
    case HT_THREAD_INIT: // A thread initialisation function
      INSERT_CALL(beforeThreadInit);
      break;
    case HT_JOIN: // A join function
      INSERT_CALL(beforeJoin);
      break;
    case HT_TX_START:
    case HT_TX_COMMIT:
    case HT_TX_ABORT:
    case HT_TX_READ:
    case HT_TX_WRITE:
      // TODO: use this new approach to instrument the sync operations
      hi->instrument(rtn, hi);
      break;
    default: // Something is very wrong if the code reaches here
      assert(false);
      break;
  }
}

/**
 * Inserts a noise-injecting hook (callback) before a function.
 *
 * @param rtn An object representing the function.
 * @param ns A structure containing noise injection settings for the function.
 */
inline
VOID instrumentNoisePoint(RTN rtn, NoiseSettings* ns)
{
  RTN_InsertCall(
    rtn, IPOINT_BEFORE, (AFUNPTR)ns->generator,
    IARG_FAST_ANALYSIS_CALL,
    IARG_THREAD_ID,
    IARG_UINT32, ns->frequency,
    IARG_UINT32, ns->strength,
    IARG_END);
}

/**
 * Instruments an image (executable, shared object, dynamic library, ...).
 *
 * @tparam BT A type of backtraces the framework should provide.
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param img An object representing the image.
 * @param v A pointer to arbitrary data.
 */
template < BacktraceType BT, ConcurrentCoverage CC >
VOID instrumentImage(IMG img, VOID* v)
{
  // The pointer 'v' is a pointer to an object containing framework settings
  Settings* settings = static_cast< Settings* >(v);

  // Check if the image should be instrumented (will be tested many times)
  bool instrument = !settings->isExcludedFromInstrumentation(img);

  if (!instrument)
  { // The image should not be instrumented, log it for pattern debugging
    LOG("Image '" + IMG_Name(img) + "' will not be instrumented.\n");
  }
  else
  { // The image should be instrumented
    LOG("Instrumenting image '" + IMG_Name(img) + "'.\n");
  }

  if (!instrument || settings->isExcludedFromDebugInfoExtraction(img))
  { // Debugging information should not be extracted from the image
    LOG("Debugging information in image '" + IMG_Name(img)
      + "' will not be extracted.\n");
  }
  else
  { // Debugging information should be extracted from the image
    LOG("Extracting debugging information from image '" + IMG_Name(img)
      + "'.\n");

    // Open the image and extract debugging information from it
    DIE_Open(img);

    if (settings->get< bool >("show-dbg-info"))
    { // Print the extracted debugging information
      DIE_Print(img);
    }
  }

  // Helper variables
  HookInfo* hi = NULL;
  NoiseSettings* ns = NULL;
  bool instrumentReturns = false;

  // Framework settings contain information about read and write noise
  MemoryAccessInstrumentationSettings mais(settings);

  // Setup the memory access callback functions and their types
  setupMemoryAccessSettings(mais);

  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
  { // Process all sections of the image
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
    { // Process all routines of the section
      RTN_Open(rtn);

      if (settings->isNoisePoint(rtn, &ns))
      { // The routine is a noise point, need to inject noise before it
        instrumentNoisePoint(rtn, ns);
      }

      if (settings->isHook(rtn, &hi))
      { // The routine is a hook, need to insert monitoring code before it
        instrumentHook< BT, CC >(rtn, hi);
        // User may use this to check if a function is really monitored
        LOG("  Found " + hi->type + " '" + RTN_Name(rtn) + "' in '"
          + IMG_Name(img) + "'\n");
        // Need to instrument returns in this image for after calls to work
        instrumentReturns = true;
      }

      if (RTN_Name(rtn) == "__cxa_throw")
      { // Insert a hook (callback) before throwing an exception
        RTN_InsertCall(
          rtn, IPOINT_BEFORE, (AFUNPTR)beforeThrow,
          IARG_THREAD_ID,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
          IARG_END);
      }
      else if (RTN_Name(rtn) == "__cxa_begin_catch")
      { // Insert a hook (callback) after a catch block is entered
        RTN_InsertCall(
          rtn, IPOINT_AFTER, (AFUNPTR)afterBeginCatch,
          IARG_THREAD_ID,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
          IARG_CONST_CONTEXT,
          IARG_END);
      }

      if (instrument && mais.instrument)
      { // Instrument all accesses (reads and writes) in the current routine
        for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
        { // Windows 64-bit do not use base pointer chains to form stack frames
#if defined(TARGET_IA32) || defined(TARGET_LINUX)
          if (BT & BT_LIGHTWEIGHT)
          { // Track stack frames to obtain the return addresses when needed
            instrumentStackFrameOperation(ins);
          }
#endif
          // Check if the instruction accesses memory and instrument it if yes
          instrumentMemoryAccess(ins, mais);
        }
      }

      // Close the routine before processing the next one
      RTN_Close(rtn);
    }
  }

  // If the returns do not need to be instrumented, we are done
  if (!instrumentReturns) return;

  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
  { // Process all sections of the image
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
    { // Process all routines of the section
      RTN_Open(rtn);

      for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
      { // Process all instructions in the routine
        if (INS_IsRet(ins))
        { // After calls are performed just before returning from a function
          INS_InsertCall(
            ins, IPOINT_BEFORE, (AFUNPTR)beforeReturn,
            CBSTACK_IARG_PARAMS,
            IARG_FUNCRET_EXITPOINT_REFERENCE,
            IARG_END);
        }
      }

      // Close the routine before processing the next one
      RTN_Close(rtn);
    }
  }
}

/**
 * Instruments a routine.
 *
 * @param rtn An object representing the routine.
 * @param v A pointer to arbitrary data.
 */
VOID instrumentRoutine(RTN rtn, VOID *v)
{
  // Routine needs to be opened before its instructions can be instrumented
  RTN_Open(rtn);

  RTN_InsertCall(
    rtn, IPOINT_BEFORE, (AFUNPTR)beforeFunctionExecuted,
    IARG_FAST_ANALYSIS_CALL,
    IARG_THREAD_ID,
    IARG_ADDRINT, indexFunction(IMG_Name(SEC_Img(RTN_Sec(rtn))) + "!"
      + RTN_Name(rtn)),
    IARG_END);

  // We are done with the instrumentation here, close the routine
  RTN_Close(rtn);
}

/**
 * Instruments a long jump routine.
 *
 * @param rtn An object representing a routine.
 * @param v A pointer to arbitrary data.
 */
VOID instrumentLongJump(RTN rtn, VOID *v)
{
  // TODO: Merge into instrumentRoutine
  if (RTN_Name(rtn) != "__longjmp") return;

  // Routine needs to be opened before its instructions can be instrumented
  RTN_Open(rtn);

  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
  { // Find the instruction restoring the stack pointer
	if (INS_RegWContain(ins, REG_STACK_PTR))
    { // We are interested in the new value of the stack pointer
      INS_InsertCall(
        ins, IPOINT_AFTER, (AFUNPTR)g_afterStackPtrSetByLongJump,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
        IARG_END);
    }
  }

  // We are done with the instrumentation here, close the routine
  RTN_Close(rtn);
}

/**
 * Cleans up and frees all resources allocated by the ANaConDA framework.
 *
 * @note This function is called when the program being analysed exits.
 *
 * @param code An OS specific termination code of the program.
 * @param v A pointer to arbitrary data.
 */
void onProgramExit(INT32 code, VOID* v)
{
  // The pointer 'v' is a pointer to an object containing framework settings
  Settings* settings = static_cast< Settings* >(v);

  // Finalise the analyser, free resources used to load the settings, etc.
  delete settings;
}

// Helper macros used in the main method only
#define BACKTRACE_TYPE(type) \
  settings->get< std::string >("backtrace.type") == type
#define BACKTRACE_VERBOSITY(verbosity) \
  settings->get< std::string >("backtrace.verbosity") == verbosity

/**
 * Determines which type of backtraces should the framework provide and setups
 *   the framework appropriately based on this knowledge.
 *
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
template< ConcurrentCoverage CC >
inline
void setupInstrumentation(Settings* settings)
{
  // Instrument the program to be analysed with appropriate backtrace support
  if (BACKTRACE_TYPE("precise"))
  { // Create backtraces consisting of call addresses
    IMG_AddInstrumentFunction(instrumentImage< BT_PRECISE, CC >,
      static_cast< VOID* >(settings));

    // Here PIN ensures that each instruction will be instrumented only once
    INS_AddInstrumentFunction(BACKTRACE_VERBOSITY("detailed") ?
      instrumentCallStackOperation< BV_DETAILED > :
      instrumentCallStackOperation< BV_MINIMAL >, 0);
  }
  else if (BACKTRACE_TYPE("full"))
  { // Create backtraces consisting of function names
    IMG_AddInstrumentFunction(instrumentImage< BT_FULL, CC >,
      static_cast< VOID* >(settings));
  }
  else if (BACKTRACE_TYPE("lightweight"))
  { // Create backtraces consisting of return addresses
    IMG_AddInstrumentFunction(instrumentImage< BT_LIGHTWEIGHT, CC >,
      static_cast< VOID* >(settings));
  }
  else
  { // No not create any backtraces (disable backtrace support)
    IMG_AddInstrumentFunction(instrumentImage< BT_NONE, CC >,
      static_cast< VOID* >(settings));
  }
}

// We need to call this type of function for every combination of CC flags
typedef void (*INSTRUMENTFUNPTR)(Settings* settings);

// For some (unknown) reason GCC needs this to find out make_table is template
template< typename >
struct make_table;

// We need to have a concrete function for each combination of CC flags, all
// combinations will be given as an integer sequence (each item on combination)
template< int... N >
struct make_table< seq< N... > >
{
  static const INSTRUMENTFUNPTR funcs[sizeof... (N)];
};

// Now we need to instantiate a function for each combination of CC flags, easy
template< int... N >
const INSTRUMENTFUNPTR make_table< seq< N... > >::funcs[sizeof... (N)] = {
  &setupInstrumentation< static_cast< ConcurrentCoverage >(N) >...
};

/**
 * Instruments and runs a program to be analysed. Also initialises the PIN
 *   dynamic instrumentation framework.
 *
 * @param argc A number of arguments passed to the PIN run script.
 * @param argv A list of arguments passed to the PIN run script.
 * @return @em 0 if the program was executed successfully.
 */
int main(int argc, char* argv[])
{
  // Needed for retrieving info about source file and line and column numbers
  PIN_InitSymbols();

  // Initialise the PIN dynamic instrumentation framework
  PIN_Init(argc, argv);

  // Register the ANaConDA framework's build-in function argument mappers
  REGISTER_MAPPER("addr", AddressFuncArgMapper);

  // Register the ANaConDA framework's build-in noise injection functions
  registerBuiltinNoiseFunctions();

  // An object containing the ANaConDA framework's settings
  Settings* settings = new Settings();

  // Register parts of the framework that need to be setup
  settings->registerSetupFunction(setupRandomModule);
  settings->registerSetupFunction(setupAccessModule);
  settings->registerSetupFunction(setupNoiseModule);
  settings->registerSetupFunction(setupSyncModule);
  settings->registerSetupFunction(setupTmModule);

  try
  { // Load the ANaConDA framework's settings
    settings->load(argc, argv);

    // Setup the ANaConDA framework's settings
    settings->setup();
  }
  catch (std::exception& e)
  { // The settings contain some error, print its description
    CONSOLE_NOPREFIX("error: " + std::string(e.what()) + "\n");
    // Some required settings are missing, do not continue
    return EXIT_FAILURE;
  }

  if (settings->get< bool >("show-settings"))
  { // Print ANaConDA framework's settings
    settings->print();
  }

  if (settings->get< bool >("coverage.predecessors"))
  { // Monitor predecessors
    g_beforeFunctionCalled = (AFUNPTR)beforeFunctionCalled< CC_PREDS >;
    g_beforeFunctionReturned = (AFUNPTR)beforeFunctionReturned< CC_PREDS >;
    g_afterStackPtrSetByLongJump = (AFUNPTR)afterStackPtrSetByLongJump< CC_PREDS >;
  }
  else
  { // Do not monitor predecessors
    g_beforeFunctionCalled = (AFUNPTR)beforeFunctionCalled< CC_NONE >;
    g_beforeFunctionReturned = (AFUNPTR)beforeFunctionReturned< CC_NONE >;
    g_afterStackPtrSetByLongJump = (AFUNPTR)afterStackPtrSetByLongJump< CC_NONE >;
  }

  // We will need to access this monitor if predecessor noise is used
  g_predsMon = &settings->getCoverageMonitors().preds;

  // Register callback functions called when a new thread is started
  PIN_AddThreadStartFunction(createCallbackStack, 0);
  PIN_AddThreadStartFunction(initMemoryAccessTls, 0);
  PIN_AddThreadStartFunction(threadStarted, 0);

  // Register callback functions called when an existing thread finishes
  PIN_AddThreadFiniFunction(threadFinished, 0);

  // Register callback functions called when the program to be analysed exits
  PIN_AddFiniFunction(onProgramExit, static_cast< VOID* >(settings));

  // Register appropriate functions for retrieving backtraces
  setupBacktraceSupport(settings);

  // Call the function supporting the chosen types of concurrent coverage
  make_table< gens< 2 >::type >::funcs[
    settings->get< bool >("coverage.synchronisation")
  ](settings);

#if ANACONDA_PRINT_EXECUTED_FUNCTIONS == 1
  // Instrument first instructions of all functions to print info about them
  RTN_AddInstrumentFunction(instrumentRoutine, 0);
#endif
  RTN_AddInstrumentFunction(instrumentLongJump, 0);

  // Run the instrumented version of the program to be analysed
  PIN_StartProgram();

  // Program finished its run, no post-execution tasks needed here for now
  return 0;
}

/** End of file anaconda.cpp **/
