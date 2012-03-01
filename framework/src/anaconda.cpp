/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2012-03-01
 * @version   0.6.2
 */

#include <assert.h>

#include <map>

#include <boost/assign/list_of.hpp>

#include "pin.H"

#include "pin_die.h"

#include "cbstack.h"
#include "mapper.h"
#include "settings.h"

#include "callbacks/access.h"
#include "callbacks/exception.h"
#include "callbacks/noise.h"
#include "callbacks/sync.h"
#include "callbacks/thread.h"

// Macro definitions
#define INSERT_CALL(callback) \
  RTN_InsertCall( \
    rtn, IPOINT_BEFORE, (AFUNPTR)callback, \
    CBSTACK_IARG_PARAMS, \
    IARG_FUNCARG_ENTRYPOINT_REFERENCE, desc->lock - 1, \
    IARG_PTR, desc, \
    IARG_END)

namespace
{ // Static global variables (usable only within this module)
  std::map< NoiseType, AFUNPTR >
    g_noiseInjectFuncMap = boost::assign::map_list_of
      (NOISE_SLEEP, (AFUNPTR)injectSleep)
      (NOISE_YIELD, (AFUNPTR)injectYield);
}

// Type definitions
typedef VOID (*INSERTCALLFUNPTR)(INS ins, IPOINT ipoint, AFUNPTR funptr, ...);

/**
 * @brief A structure containing instrumentation settings.
 */
typedef struct InstrumentationSettings_s
{
  AFUNPTR beforeCallback; //!< A function called before an instrumented object.
  AFUNPTR afterCallback; //!< A function called after an instrumented object.
  /**
   * @brief A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  NoiseDesc* noise;

  /**
   * Constructs an InstrumentationSettings_s object.
   */
  InstrumentationSettings_s() : beforeCallback(NULL), afterCallback(NULL),
    noise(NULL) {}

  /**
   * Constructs an InstrumentationSettings_s object.
   *
   * @param n A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  InstrumentationSettings_s(NoiseDesc* n) : beforeCallback(NULL),
    afterCallback(NULL), noise(n) {}
} InstrumentationSettings;

/**
 * @brief A structure containing memory access instrumentation settings.
 */
typedef struct MemoryAccessInstrumentationSettings_s
{
  /**
   * @brief A structure describing how to instrument instructions reading from
   *   a memory.
   */
  InstrumentationSettings reads;
  /**
   * @brief A structure describing how to instrument instructions writing to
   *   a memory.
   */
  InstrumentationSettings writes;

  /**
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   */
  MemoryAccessInstrumentationSettings_s() : reads(), writes() {}

  /**
   * Constructs a MemoryAccessSettings_s object.
   *
   * @param s An object containing the ANaConDA framework's settings.
   */
  MemoryAccessInstrumentationSettings_s(Settings* s) : reads(s->getReadNoise()),
   writes(s->getWriteNoise()) {}
} MemoryAccessInstrumentationSettings;

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
  InstrumentationSettings& access = mais.reads;

  // Predicated instruction might not be executed at all
  if (INS_IsPredicated(ins)) insertCall = INS_InsertPredicatedCall;

  for (UINT32 memOpIdx = 0; memOpIdx < memOpCount; memOpIdx++)
  { // Instrument all memory accesses (reads and writes)
    if (INS_MemoryOperandIsWritten(ins, memOpIdx))
    { // The memOpIdx-th memory access is a write access
      access = mais.writes;
    }
    else
    { // The memOpIdx-th memory access is a read access
      access = mais.reads;
    }

    insertCall(
      ins, IPOINT_BEFORE, access.beforeCallback,
      IARG_THREAD_ID,
      IARG_MEMORYOP_EA, memOpIdx,
      IARG_UINT32, INS_MemoryOperandSize(ins, memOpIdx),
      IARG_UINT32, memOpIdx,
      IARG_ADDRINT, RTN_Address(INS_Rtn(ins)),
      IARG_ADDRINT, INS_Address(ins),
      IARG_CONST_CONTEXT,
      IARG_END);
    insertCall(
      ins, IPOINT_BEFORE, g_noiseInjectFuncMap[access.noise->type],
      IARG_UINT32, access.noise->frequency,
      IARG_UINT32, access.noise->strength,
      IARG_END);
    insertCall(
      ins, IPOINT_AFTER, access.afterCallback,
      IARG_THREAD_ID,
      IARG_UINT32, memOpIdx,
      IARG_END);
  }
}

/**
 * Instruments all memory accesses (reads and writes) of a function.
 *
 * @param rtn A function whose memory accesses should be instrumented.
 * @param settings An object containing the ANaConDA framework's settings.
 */
inline
VOID instrumentMemoryAccesses(RTN rtn, Settings* settings)
{
  // Setup the whole process of instrumenting memory accesses
  MemoryAccessInstrumentationSettings mais(settings);

  // Setup the functions called before and after memory accesses
  mais.reads.beforeCallback = (AFUNPTR)beforeMemoryRead;
  mais.reads.afterCallback = (AFUNPTR)afterMemoryRead;
  mais.writes.beforeCallback = (AFUNPTR)beforeMemoryWrite;
  mais.writes.afterCallback = (AFUNPTR)afterMemoryWrite;

  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
  { // Process all instructions in the routine
    instrumentMemoryAccess(ins, mais);
  }
}

/**
 * Inserts hooks around (before and after) a synchronisation function.
 *
 * @param rtn An object representing the function.
 * @param desc A structure containing the description of the synchronisation
 *   function.
 */
inline
VOID instrumentSyncFunction(RTN rtn, FunctionDesc* desc)
{
  switch (desc->type)
  { // Instrument the function based on its type
    case FUNC_LOCK: // A lock function
      INSERT_CALL(beforeLockAcquire);
      break;
    case FUNC_UNLOCK: // An unlock function
      INSERT_CALL(beforeLockRelease);
      break;
    case FUNC_SIGNAL: // A signal function
      INSERT_CALL(beforeSignal);
      break;
    case FUNC_WAIT: // A wait function
      INSERT_CALL(beforeWait);
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
 * @param desc A structure containing the description of the noise which should
 *   be inserted before the function.
 */
inline
VOID instrumentNoisePoint(RTN rtn, NoiseDesc* desc)
{
  RTN_InsertCall(
    rtn, IPOINT_BEFORE, g_noiseInjectFuncMap[desc->type],
    IARG_UINT32, desc->frequency,
    IARG_UINT32, desc->strength,
    IARG_END);
}

/**
 * Instruments an image (executable, shared object, dynamic library, ...).
 *
 * @param img An object representing the image.
 * @param v A pointer to arbitrary data.
 */
VOID image(IMG img, VOID* v)
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

#ifdef DEBUG
    // Print the extracted debugging information
    DIE_Print(img);
#endif
  }

  // Helper variables
  NoiseDesc* noiseDesc = NULL;
  FunctionDesc* funcDesc = NULL;
  bool instrumentReturns = false;

  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
  { // Process all sections of the image
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
    { // Process all routines of the section
      RTN_Open(rtn);

      if (settings->isNoisePoint(rtn, &noiseDesc))
      { // The routine is a noise point, need to inject noise before it
        instrumentNoisePoint(rtn, noiseDesc);
      }

      if (settings->isSyncFunction(rtn, &funcDesc))
      { // The routine is a sync function, need to insert hooks around it
        instrumentSyncFunction(rtn, funcDesc);
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

      if (instrument)
      { // Instrument all accesses (reads and writes) in the current routine
        instrumentMemoryAccesses(rtn, settings);
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
          RTN_InsertCall(
            rtn, IPOINT_BEFORE, (AFUNPTR)beforeReturn,
            CBSTACK_IARG_PARAMS,
            IARG_END);
        }
      }

      // Close the routine before processing the next one
      RTN_Close(rtn);
    }
  }
}

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

  // An object containing the ANaConDA framework's settings
  Settings* settings = new Settings();

  // Load the ANaConDA framework's settings
  settings->load(argc, argv);

#ifdef DEBUG
  // Print ANaConDA framework's settings
  settings->print();
#endif

  // Register callback functions called when a new thread is started
  PIN_AddThreadStartFunction(createCallbackStack, 0);
  PIN_AddThreadStartFunction(initSyncFunctionTls, 0);
  PIN_AddThreadStartFunction(initAccessTls, 0);
  PIN_AddThreadStartFunction(threadStarted, 0);

  // Register callback functions called when an existing thread finishes
  PIN_AddThreadFiniFunction(threadFinished, 0);

  // Instrument the program to be analysed
  IMG_AddInstrumentFunction(image, static_cast< VOID* >(settings));

  // Run the instrumented version of the program to be analysed
  PIN_StartProgram();

  // Free all the previously allocated memory
  delete settings;

  // Program finished its run, no post-execution tasks needed here for now
  return 0;
}

/** End of file anaconda.cpp **/
