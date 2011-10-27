/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2011-10-27
 * @version   0.1.3
 */

#include <boost/lexical_cast.hpp>

#include "pin.H"

#include "pin_die.h"

#include "settings.h"

#include "callbacks/access.h"

#define LOG_IMPLICIT_OPERAND_READS

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

  if (settings->isExcludedFromInstrumentation(img))
  { // The image should not be instrumented, log it for pattern debugging
    LOG("Image '" + IMG_Name(img) + "' will not be instrumented.\n");
    LOG("Debugging information in image '" + IMG_Name(img)
      + "' will not be extracted.\n");

    return;
  }

  // The image should be instrumented
  LOG("Instrumenting image '" + IMG_Name(img) + "'.\n");

  if (settings->isExcludedFromDebugInfoExtraction(img))
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

  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
  { // Process all sections of the image
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
    { // Process all routines of the section
      RTN_Open(rtn);

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
              INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)beforeMemoryWriteValue,
                IARG_ADDRINT, RTN_Address(rtn),
                IARG_ADDRINT, INS_Address(ins),
                IARG_MEMORYWRITE_EA,
                IARG_MEMORYWRITE_SIZE,
                IARG_REG_VALUE, INS_OperandReg(ins, op),
                IARG_CONST_CONTEXT,
                IARG_END);
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
  // An object containing the ANaConDA framework settings
  Settings *settings = new Settings();

  // Load the ANaConDA framework settings
  settings->load();

#ifdef DEBUG
  // Print ANaConDA framework settings
  settings->print();
#endif

  // Needed for retrieving info about source file and line and column numbers
  PIN_InitSymbols();

  // Initialise the PIN dynamic instrumentation framework
  PIN_Init(argc, argv);

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
