/*
 * Copyright (C) 2011-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2020-04-14
 * @version   0.17
 */

#include <assert.h>

#ifdef TARGET_LINUX
  #include <unistd.h>
  #include <fcntl.h>
#endif

#include "pin.H"

#include "libdie/version.h"

#include "libdie-wrapper/pin_die.h"

#include "anaconda.h"
#include "cbstack.h"
#include "config.h"
#include "index.h"
#include "mapper.h"
#include "settings.h"
#include "version.h"

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

namespace
{ // Static global variables (usable only within this module)
  PredecessorsMonitor< FileWriter >* g_predsMon;

#ifdef TARGET_LINUX
  int g_origStdout;
  int g_origStderr;
#endif
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
VOID PIN_FAST_ANALYSIS_CALL br(THREADID tid, ADDRINT sp, ADDRINT idx)
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

  // Static (non-changing) information about the instruction accessing memory
  MemoryAccessInstructionInfo* memAccInsInfo = new MemoryAccessInstructionInfo(
    INS_Address(ins), RTN_Address(INS_Rtn(ins)));

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

    // Static (non-changing) information about the memory access
    MemoryAccessInfo* memAccInfo = new MemoryAccessInfo(memOpIdx,
      INS_MemoryOperandSize(ins, memOpIdx), memAccInsInfo);

    if (INS_HasRealRep(ins))
    { // Do not use predicated calls for REP instructions (they seems broken)
      if (access->beforeRepAccess != NULL)
        INS_InsertCall(
          ins, IPOINT_BEFORE, access->beforeRepAccess,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_MEMORYOP_EA, memOpIdx,
          IARG_CONST_CONTEXT,
          IARG_EXECUTING,
          IARG_PTR, memAccInfo,
          IARG_END);
      if (access->afterRepAccess != NULL)
        INS_InsertCall(
          ins, IPOINT_AFTER, access->afterRepAccess,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_PTR, memAccInfo,
          IARG_END);
    }
    else
    { // Use predicated calls for conditional instructions, normal for others
      if (access->beforeAccess != NULL)
        insertCall(
          ins, IPOINT_BEFORE, access->beforeAccess,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_MEMORYOP_EA, memOpIdx,
          IARG_CONST_CONTEXT,
          IARG_PTR, memAccInfo,
          IARG_END);
      if (access->afterAccess != NULL)
        insertCall(
          ins, IPOINT_AFTER, access->afterAccess,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_PTR, memAccInfo,
          IARG_END);
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
 * Inserts a noise-injecting hook (callback) before an instruction.
 *
 * @param ins An object representing the instruction.
 * @param ns A structure containing the description of the noise to be injected
 *   before the instruction.
 */
inline
VOID instrumentNoisePoint(INS ins, NoiseSettings* ns)
{
  if (ns->frequency == 0) return; // Do not inject any noise before instructions

  INS_InsertCall(
    ins, IPOINT_BEFORE, (AFUNPTR)ns->generator,
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
  HookInfoList* hl = NULL;
  NoiseSettings* ns = NULL;
  bool instrumentReturns = false;

  // Helper type definitions
  typedef struct FilterData_s {
    struct {
      Settings::FilterResult image;
      Settings::FilterResult function;
    } reason;
    bool disable;
  } FilterData;

  // Structure containing information about the results of various filters
  struct {
    FilterData access; // Filter controlling the monitoring of memory accesses
  } filter;

  // Check if we should disable monitoring of memory accesses in this image
  filter.access.disable = settings->disableMemoryAccessMonitoring(img,
    filter.access.reason.image);

  // Framework settings contain information about read and write noise
  MemoryAccessSettings mas(settings);

  // Setup the memory access callback functions and their types
  setupMemoryAccessSettings(mas);

  if (instrument && mas.instrument && !filter.access.disable)
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

      if (settings->isHook(rtn, &hl))
      { // The routine is a hook, need to insert monitoring code before it
        BOOST_FOREACH(HookInfo* hi, *hl)
        { // Each routine can act as more than one hook at the same time
          assert(hi->instrument != NULL);
          // Insert specific monitoring code for a specific type of hook
          hi->instrument(rtn, hi);
          // User may use this to check if a function is really monitored
          LOG("  [+] Found a " + hi->type + " " + RTN_Name(rtn) + "\n");
        }
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

      if (instrument && mas.instrument && !filter.access.disable)
      { // Check if we should monitor memory accesses in this function
        if (settings->disableMemoryAccessMonitoring(rtn,
          filter.access.reason.function, filter.access.reason.image))
        { // Do not monitor memory accesses in this particular function
          LOG("  [-] Memory accesses in function " + RTN_Name(rtn)
            + " will not be monitored.\n");
        }
        else
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
  if (Settings::Get()->isExcludedFromMonitoring(rtn))
  { // Do not instrument functions that should not be monitored
    LOG("  [-] Execution of function " + RTN_Name(rtn)
      + " will not be monitored.\n");

    return;
  }

  // Routine needs to be opened before its instructions can be instrumented
  RTN_Open(rtn);

  RTN_InsertCall(
    rtn, IPOINT_BEFORE, (AFUNPTR)beforeFunctionExecuted,
    IARG_FAST_ANALYSIS_CALL,
    IARG_THREAD_ID,
    IARG_REG_VALUE, REG_STACK_PTR,
    IARG_ADDRINT, indexFunction(rtn),
    // Hook callback functions may use the data updated by the above function,
    // so call the above callback function before the hook callback functions
    IARG_CALL_ORDER, CALL_ORDER_DEFAULT - 10,
    IARG_END);

  // We are done with the instrumentation here, close the routine
  RTN_Close(rtn);
}

/**
 * Instruments an instruction.
 *
 * @param rtn An object representing the instruction.
 * @param v A pointer to arbitrary data.
 */
VOID instrumentInstruction(INS ins, VOID *v)
{
  // Helper variables
  NoiseSettings* ns = NULL;
  LOCATION location;

  // Get the location in the source code for which was the instruction generated
  PIN_GetSourceLocation(INS_Address(ins), NULL, &location.line, &location.file);

  if (Settings::Get()->isNoisePoint(&location, &ns))
  { // The instruction is a noise point, need to inject noise before it
    instrumentNoisePoint(ins, ns);
    // Let the user know that a noise will be inserted before this instruction
    LOG("  [+] Found a noise point at location " + location + "\n");
  }
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
#ifdef TARGET_LINUX
  // Make sure stdout and stderr are still usable before shutting down analysers
  // as the analysers may need to output something in their finish functions
  if (fcntl(STDOUT_FILENO, F_GETFD) == -1)
  { // Stdout already closed, restore it
    dup2(g_origStdout, STDOUT_FILENO);
  }

  if (fcntl(STDERR_FILENO, F_GETFD) == -1)
  { // Stderr already closed, restore it
    dup2(g_origStderr, STDERR_FILENO);
  }
#endif

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

  // TODO: Call this only when the location noise is in use
  INS_AddInstrumentFunction(instrumentInstruction, 0);
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
  { // Print detailed version information
    CONSOLE_NOPREFIX("ANaConDA " + std::string(ANACONDA_GetVersionLong()) + "\n"
      + "  using libdie " + std::string(DIE_GetVersionLong()) + "\n\n");

    // Print ANaConDA framework's settings
    settings->print();

    CONSOLE_NOPREFIX("\n");
  }
  else
  { // Print brief version information
    CONSOLE_NOPREFIX("ANaConDA " + std::string(ANACONDA_GetVersion()) + "\n"
      + "  using libdie " + std::string(DIE_GetVersion()) + "\n\n");
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

#ifdef TARGET_LINUX
  // Save the stdout and stderr for later use
  g_origStdout = dup(STDOUT_FILENO);
  g_origStderr = dup(STDERR_FILENO);
#endif

  // Run the instrumented version of the program to be analysed
  PIN_StartProgram();

  // Program finished its run, no post-execution tasks needed here for now
  return 0;
}

/** End of file anaconda.cpp **/
