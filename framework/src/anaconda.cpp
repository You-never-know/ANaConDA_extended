/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2016-05-27
 * @version   0.14.7
 */

#include <assert.h>

#include "pin.H"

#include "pin_die.h"

#include "anaconda.h"
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
#define MAS_BEFORE_STD_CALLBACK beforeAccess
#define MAS_AFTER_STD_CALLBACK afterAccess
#define MAS_BEFORE_REP_CALLBACK beforeRepAccess
#define MAS_AFTER_REP_CALLBACK afterRepAccess

#define INSERT_CALL_STD insertCall
#define INSERT_CALL_REP INS_InsertCall

// Helper macros for instrumenting memory accesses
#define INSTRUMENT_MEMORY_ACCESS(where, type) \
  if (access->MAS_##where##_##type##_CALLBACK != NULL) \
    INSERT_CALL_##type( \
      ins, IPOINT_##where, access->MAS_##where##_##type##_CALLBACK, \
      IARG_FAST_ANALYSIS_CALL, \
      where##_##type##_MEMORY_ACCESS_IARG_PARAMS, \
      IARG_END)

namespace
{ // Static global variables (usable only within this module)
  PredecessorsMonitor< FileWriter >* g_predsMon;
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

// FIXME: Temporary helper function
VOID br(THREADID tid, ADDRINT sp, ADDRINT idx)
{
  beforeFunctionReturned(tid, sp, idx);
  cbstack::beforeReturn(tid, sp, NULL);
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
        ins, IPOINT_BEFORE, (AFUNPTR)beforeFunctionCalled,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
        IARG_ADDRINT, indexCall(ins),
        IARG_END);
      break;
    case XED_ICLASS_RET_FAR:
    case XED_ICLASS_RET_NEAR:
      INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)br,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
        IARG_ADDRINT, indexInstruction(ins),
        IARG_END);
      break;
    default: // Make sure we do not miss any calls or returns
      assert(!INS_IsCall(ins));
      assert(!INS_IsRet(ins));
      break;
  }
}

/**
 * Instruments all memory accesses (reads and writes) of an instruction.
 *
 * @param ins An instruction whose memory accesses should be instrumented.
 * @param mas An object containing memory access instrumentation settings.
 */
inline
VOID instrumentMemoryAccess(INS ins, MemoryAccessSettings& mas)
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

  if (INS_Opcode(ins) == XED_ICLASS_JMP || INS_Opcode(ins) == XED_ICLASS_JMP_FAR)
  { // Do not instrument jumps reading the target address from a memory, they
    // are not fall-through and read from read-only parts (see ISDM-2A 3-556)
    assert(memOpCount == 1 && INS_MemoryOperandIsRead(ins, 0));
    return;
  }

  // Just to be sure that we will be able to insert the after calls
  assert(INS_HasFallThrough(ins));

  // Helper variables (better than having 4 nearly same blocks of code)
  INSERTCALLFUNPTR insertCall = INS_InsertCall;
  MemoryAccessInstrumentationSettings* access = NULL;

  // Predicated instruction might not be executed at all
  if (INS_IsPredicated(ins)) insertCall = INS_InsertPredicatedCall;

  for (UINT32 memOpIdx = 0; memOpIdx < memOpCount; memOpIdx++)
  { // Instrument all memory accesses (reads and writes)
    if (INS_MemoryOperandIsWritten(ins, memOpIdx))
    { // The memOpIdx-th memory access is a write or update access
      access = (INS_MemoryOperandIsRead(ins, memOpIdx))
        ? &mas.updates : &mas.writes;
    }
    else
    { // The memOpIdx-th memory access is a read access
      access = &mas.reads;
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
 * Inserts a noise-injecting hook (callback) before a function.
 *
 * @param rtn An object representing the function.
 * @param ns A structure containing noise injection settings for the function.
 */
inline
VOID instrumentNoisePoint(RTN rtn, NoiseSettings* ns)
{
  if (ns->frequency == 0) return; // Do not inject any noise before functions

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
  // Print basic information about the image to be processed
  LOG("Processing image " + IMG_Name(img) + "\n");
  LOG("  Loaded at memory range "
    + hexstr(IMG_LowAddress(img)) + ":"
    + hexstr(IMG_HighAddress(img)) + "\n");
  LOG("  Processing details:\n");

  // The pointer 'v' is a pointer to an object containing framework settings
  Settings* settings = static_cast< Settings* >(v);

  // Check if the image should be instrumented (will be tested many times)
  bool instrument = !settings->isExcludedFromInstrumentation(img);

  if (!instrument)
  { // The image should not be instrumented, log it for pattern debugging
    LOG("  [ ] Image will not be instrumented.\n");
  }
  else
  { // The image should be instrumented
    LOG("  [X] Image will be instrumented.\n");
  }

  if (!instrument || settings->isExcludedFromDebugInfoExtraction(img))
  { // Debugging information should not be extracted from the image
    LOG("  [ ] Debugging information will not be extracted.\n");
  }
  else
  { // Debugging information should be extracted from the image
    LOG("  [X] Debugging information will be extracted.\n");

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
  MemoryAccessSettings mas(settings);

  // Setup the memory access callback functions and their types
  setupMemoryAccessSettings(mas);

  if (instrument && mas.instrument)
  { // Instrumentation enabled and at least one access callback is registered
    LOG("  [X] Memory accesses will be instrumented.\n");
  }
  else
  { // Do not instrument at all or no memory access information is required
    LOG("  [ ] Memory accesses will not be instrumented.\n");
  }

  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
  { // Process all sections of the image
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
    { // Process all routines of the section
      RTN_Open(rtn);

      if (settings->isNoisePoint(rtn, &ns))
      { // The routine is a noise point, need to inject noise before it
        instrumentNoisePoint(rtn, ns);
        // Let the user know that a noise will be inserted before this function
        LOG("  [+] Found a noise point " + RTN_Name(rtn) + "\n");
      }

      if (settings->isHook(rtn, &hi))
      { // The routine is a hook, need to insert monitoring code before it
        hi->instrument(rtn, hi);
        // User may use this to check if a function is really monitored
        LOG("  [+] Found a " + hi->type + " " + RTN_Name(rtn) + "\n");
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

      if (instrument && mas.instrument)
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
          instrumentMemoryAccess(ins, mas);
        }
      }

      // Close the routine before processing the next one
      RTN_Close(rtn);
    }
  }

  if (!instrumentReturns)
  { // The returns do not need to be instrumented, we are done here
    LOG("  [ ] Returns will not be instrumented.\n");

    return;
  }

  // Find and instrument all returns found in the image
  LOG("  [X] Returns will be instrumented.\n");

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
            ins, IPOINT_BEFORE, (AFUNPTR)cbstack::beforeReturn,
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
  // Do not instrument functions that should not be monitored
  if (Settings::Get()->isExcludedFromMonitoring(rtn)) return;

  // Routine needs to be opened before its instructions can be instrumented
  RTN_Open(rtn);

  RTN_InsertCall(
    rtn, IPOINT_BEFORE, (AFUNPTR)beforeFunctionExecuted,
    IARG_FAST_ANALYSIS_CALL,
    IARG_THREAD_ID,
    IARG_REG_VALUE, REG_STACK_PTR,
    IARG_ADDRINT, indexFunction(rtn),
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
        ins, IPOINT_AFTER, (AFUNPTR)afterStackPtrSetByLongJump,
        IARG_FAST_ANALYSIS_CALL,
        IARG_THREAD_ID,
        IARG_REG_VALUE, REG_STACK_PTR,
        IARG_END);
      INS_InsertCall(
        ins, IPOINT_AFTER, (AFUNPTR)cbstack::beforeLongJump,
        CBSTACK_IARG_PARAMS,
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

    // Instrument the beginning of each function, there we can get its name
    RTN_AddInstrumentFunction(instrumentRoutine, 0);
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

  // Get the object containing the ANaConDA framework's settings
  Settings* settings = Settings::Get();

  // TODO: Transform to standard module setup function like the ones below
  setupIndexModule();

  // Register parts of the framework that need to be setup
  settings->registerSetupFunction(setupRandomModule);
  settings->registerSetupFunction(setupThreadModule);
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

  // Call the function supporting the chosen types of concurrent coverage
  make_table< gens< 2 >::type >::funcs[
    settings->get< bool >("coverage.synchronisation")
  ](settings);

  if (settings->get< bool >("coverage.synchronisation"))
  { // The framework should monitor the synchronisation coverage, enable it
    static SyncCoverageMonitor< FileWriter >&
      syncMon = settings->getCoverageMonitors().sync;

    // Cannot call the monitor methods directly, wrap the calls into lambdas
    // (using captures would make the lambdas incompatible with the standard
    // functions we need to register, so we use a static reference instead)
    SYNC_BeforeLockAcquire((LOCKFUNPTR)([] (THREADID tid, LOCK lock) -> VOID
      { syncMon.beforeLockAcquired(tid, lock); }));
    SYNC_AfterLockAcquire((LOCKFUNPTR)([] (THREADID tid, LOCK lock) -> VOID
      { syncMon.afterLockAcquired(tid, lock); }));
    SYNC_BeforeLockRelease((LOCKFUNPTR)([] (THREADID tid, LOCK lock) -> VOID
      { syncMon.beforeLockReleased(tid, lock); }));
  }

  if (settings->get< bool >("coverage.sharedvars"))
  { // The framework should monitor shared variables, enable their monitoring
    static SharedVariablesMonitor< FileWriter >&
      svarsMon = settings->getCoverageMonitors().svars;

    // Cannot call the monitor methods directly, wrap the calls into lambdas
    // (using captures would make the lambdas incompatible with the standard
    // functions we need to register, so we use a static reference instead)
    ACCESS_BeforeMemoryRead((MEMREADAVOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, BOOL isLocal) -> VOID
      { svarsMon.beforeVariableAccessed(tid, addr, variable, isLocal); }));
    ACCESS_BeforeMemoryWrite((MEMWRITEAVOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, BOOL isLocal) -> VOID
      { svarsMon.beforeVariableAccessed(tid, addr, variable, isLocal); }));
    ACCESS_BeforeAtomicUpdate((MEMUPDATEAVOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, BOOL isLocal) -> VOID
      { svarsMon.beforeVariableAccessed(tid, addr, variable, isLocal); }));
  }

  if (settings->get< bool >("coverage.predecessors"))
  { // The framework should monitor the predecessors, enable their monitoring
    static PredecessorsMonitor< FileWriter >&
      predsMon = settings->getCoverageMonitors().preds;

    // Cannot call the monitor methods directly, wrap the calls into lambdas
    // (using captures would make the lambdas incompatible with the standard
    // functions we need to register, so we use a static reference instead)
    THREAD_FunctionEntered((THREADFUNPTR)([] (THREADID tid) -> VOID
      { predsMon.beforeFunctionEntered(tid); }));
    THREAD_FunctionExited((THREADFUNPTR)([] (THREADID tid) -> VOID
      { predsMon.beforeFunctionExited(tid); }));
    ACCESS_BeforeMemoryRead((MEMREADAVIOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, ADDRINT ins,
      BOOL isLocal) -> VOID
      { predsMon.beforeVariableAccessed(tid, addr, variable, ins, isLocal); }));
    ACCESS_BeforeMemoryWrite((MEMWRITEAVIOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, ADDRINT ins,
      BOOL isLocal) -> VOID
      { predsMon.beforeVariableAccessed(tid, addr, variable, ins, isLocal); }));
    ACCESS_BeforeAtomicUpdate((MEMUPDATEAVIOFUNPTR)([] (THREADID tid,
      ADDRINT addr, UINT32 size, const VARIABLE& variable, ADDRINT ins,
      BOOL isLocal) -> VOID
      { predsMon.beforeVariableAccessed(tid, addr, variable, ins, isLocal); }));
  }

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
