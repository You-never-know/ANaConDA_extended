/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2011-11-30
 * @version   0.3
 */

#include <map>

#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

#include "pin.H"

#include "pin_die.h"

#include "mapper.h"
#include "settings.h"

#include "callbacks/access.h"
#include "callbacks/noise.h"
#include "callbacks/sync.h"

#define LOG_IMPLICIT_OPERAND_READS

namespace
{ // Static global variables (usable only within this module)
  std::map< NoiseType, AFUNPTR >
    g_noiseInjectFuncMap = boost::assign::map_list_of
      (NOISE_SLEEP, (AFUNPTR)injectSleep)
      (NOISE_YIELD, (AFUNPTR)injectYield);
}

/**
 * Instruments all accesses (reads and writes) in a function.
 *
 * @param rtn An object representing the function.
 */
inline
VOID instrumentAccesses(RTN rtn)
{
  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
  { // Process all instructions in the routine
    if (INS_MemoryOperandCount(ins) > 2)
    { // Some new instructions might have more than 2 memory operands
      LOG("Instruction '" + INS_Disassemble(ins)
        + "' has more than 2 memory operands.\n");
    }

    if (INS_HasMemoryRead2(ins))
    { // The instruction has two memory operands
      INS_InsertPredicatedCall(
        ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryRead2,
        IARG_ADDRINT, RTN_Address(rtn),
        IARG_ADDRINT, INS_Address(ins),
        IARG_MEMORYREAD_EA,
        IARG_MEMORYREAD2_EA,
        IARG_MEMORYREAD_SIZE,
        IARG_CONST_CONTEXT,
        IARG_END);
    }
    else if (INS_IsMemoryRead(ins))
    { // The instruction reads from a memory
      INS_InsertPredicatedCall(
        ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryRead,
        IARG_ADDRINT, RTN_Address(rtn),
        IARG_ADDRINT, INS_Address(ins),
        IARG_MEMORYREAD_EA,
        IARG_MEMORYREAD_SIZE,
        IARG_CONST_CONTEXT,
        IARG_END);
    }

    if (INS_IsMemoryWrite(ins))
    { // The instruction writes to a memory
      UINT32 opCount = INS_OperandCount(ins);

      for (UINT32 op = 0; op < opCount; op++)
      { // Locate the value to be written among the remaining operands
        if (!INS_OperandRead(ins, op)) continue;

        if (INS_OperandIsReg(ins, op))
        { // Instruction writes a value stored in a register to the memory
          REG reg = INS_OperandReg(ins, op);

          if (REG_is_xmm(reg))
          { // Register is a 128-bit XMM register, cannot use IARG_REG_VALUE
            INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteXmmReg,
              IARG_ADDRINT, RTN_Address(rtn),
              IARG_ADDRINT, INS_Address(ins),
              IARG_MEMORYWRITE_EA,
              IARG_MEMORYWRITE_SIZE,
              IARG_REG_REFERENCE, reg,
              IARG_CONST_CONTEXT,
              IARG_END);
          }
          else if (REG_is_ymm(reg))
          { // Register is a 256-bit YMM register, cannot use IARG_REG_VALUE
            INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteYmmReg,
              IARG_ADDRINT, RTN_Address(rtn),
              IARG_ADDRINT, INS_Address(ins),
              IARG_MEMORYWRITE_EA,
              IARG_MEMORYWRITE_SIZE,
              IARG_REG_REFERENCE, reg,
              IARG_CONST_CONTEXT,
              IARG_END);
          }
          else if (REG_is_fr_or_x87(reg))
          { // Register is a FP or x87 register, cannot use IARG_REG_VALUE
            INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteYmmReg,
              IARG_ADDRINT, RTN_Address(rtn),
              IARG_ADDRINT, INS_Address(ins),
              IARG_MEMORYWRITE_EA,
              IARG_MEMORYWRITE_SIZE,
              IARG_REG_REFERENCE, reg,
              IARG_CONST_CONTEXT,
              IARG_END);
          }
          else
          { // Register is a general purpose register, use IARG_REG_VALUE
            INS_InsertPredicatedCall(
              ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteValue,
              IARG_ADDRINT, RTN_Address(rtn),
              IARG_ADDRINT, INS_Address(ins),
              IARG_MEMORYWRITE_EA,
              IARG_MEMORYWRITE_SIZE,
              IARG_REG_VALUE, reg,
              IARG_CONST_CONTEXT,
              IARG_END);
          }
        }
        else if (INS_OperandIsImmediate(ins, op))
        { // Instruction writes an immediate value to the memory
          INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteValue,
            IARG_ADDRINT, RTN_Address(rtn),
            IARG_ADDRINT, INS_Address(ins),
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_ADDRINT, INS_OperandImmediate(ins, op),
            IARG_CONST_CONTEXT,
            IARG_END);
        }
        else if (INS_OperandIsMemory(ins, op))
        { // Instruction writes a value stored in a memory to the memory
          INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWrite,
            IARG_ADDRINT, RTN_Address(rtn),
            IARG_ADDRINT, INS_Address(ins),
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_CONST_CONTEXT,
            IARG_END);
        }
        else if (INS_OperandIsImplicit(ins, op))
        { // Instruction writes a value given by the instruction itself
#ifdef LOG_IMPLICIT_OPERAND_READS
          LOG("Implicit read operand [" + boost::lexical_cast< string >(op)
              + "] in instruction '" + INS_Disassemble(ins) + "'\n");
#endif
          INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteValue,
            IARG_ADDRINT, RTN_Address(rtn),
            IARG_ADDRINT, INS_Address(ins),
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_ADDRINT, 0,
            IARG_CONST_CONTEXT,
            IARG_END);
        }
        else
        { // Instruction writes a value from an unknown source to the memory
          LOG("Unknown read operand [" + boost::lexical_cast< string >(op)
            + "] in instruction '" + INS_Disassemble(ins) + "'\n");
        }
      }
    }
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
      RTN_InsertCall(
        rtn, IPOINT_BEFORE, (AFUNPTR)beforeLockAcquire,
        IARG_THREAD_ID,
        IARG_FUNCARG_ENTRYPOINT_REFERENCE, desc->lock - 1,
        IARG_PTR, desc,
        IARG_END);
      RTN_InsertCall(
        rtn, IPOINT_AFTER, (AFUNPTR)afterLockAcquire,
        IARG_THREAD_ID,
        IARG_END);
      break;
    case FUNC_UNLOCK: // An unlock function
      RTN_InsertCall(
        rtn, IPOINT_BEFORE, (AFUNPTR)beforeLockRelease,
        IARG_THREAD_ID,
        IARG_FUNCARG_ENTRYPOINT_REFERENCE, desc->lock - 1,
        IARG_PTR, desc,
        IARG_END);
      RTN_InsertCall(
        rtn, IPOINT_AFTER, (AFUNPTR)afterLockRelease,
        IARG_THREAD_ID,
        IARG_END);
      break;
    case FUNC_SIGNAL: // A signal function
      RTN_InsertCall(
        rtn, IPOINT_BEFORE, (AFUNPTR)beforeSignal,
        IARG_THREAD_ID,
        IARG_FUNCARG_ENTRYPOINT_REFERENCE, desc->lock - 1,
        IARG_PTR, desc,
        IARG_END);
      RTN_InsertCall(
        rtn, IPOINT_AFTER, (AFUNPTR)afterSignal,
        IARG_THREAD_ID,
        IARG_END);
      break;
    case FUNC_WAIT: // A wait function
      RTN_InsertCall(
        rtn, IPOINT_BEFORE, (AFUNPTR)beforeWait,
        IARG_THREAD_ID,
        IARG_FUNCARG_ENTRYPOINT_REFERENCE, desc->lock - 1,
        IARG_PTR, desc,
        IARG_END);
      RTN_InsertCall(
        rtn, IPOINT_AFTER, (AFUNPTR)afterWait,
        IARG_THREAD_ID,
        IARG_END);
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
VOID image(IMG img, VOID *v)
{
  // The pointer 'v' is a pointer to an object containing framework settings
  Settings *settings = static_cast< Settings* >(v);

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
  }

  // Helper variables
  NoiseDesc* noiseDesc = NULL;
  FunctionDesc* funcDesc = NULL;

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
      }

      if (instrument)
      { // Instrument all accesses (reads and writes) in the current routine
        instrumentAccesses(rtn);
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
int main(int argc, char *argv[])
{
  // Needed for retrieving info about source file and line and column numbers
  PIN_InitSymbols();

  // Initialise the PIN dynamic instrumentation framework
  PIN_Init(argc, argv);

  // Register the ANaConDA framework's build-in function argument mappers
  REGISTER_MAPPER("addr", AddressFuncArgMapper);

  // An object containing the ANaConDA framework's settings
  Settings *settings = new Settings();

  // Load the ANaConDA framework's settings
  settings->load();

#ifdef DEBUG
  // Print ANaConDA framework's settings
  settings->print();
#endif

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
