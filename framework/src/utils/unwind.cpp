/**
 * @brief Contains implementation of functions for instrumenting unwind hooks.
 *
 * A file containing implementation of functions for instrumenting unwind hooks.
 *
 * @file      unwind.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-07
 * @date      Last Update 2016-06-07
 * @version   0.1
 */

#include "unwind.h"

#include "../index.h"

/**
 * Analyses the code of an unwind function and tries to detect where (at which
 *   instruction) the unwinding of the thread's stack is complete. Instruments
 *   this instruction to extract the new value of the stack pointer and passes
 *   this value to the callback function specified.
 *
 * @warning This function assumes that the unwind function is already opened,
 *   i.e., that @c RTN_Open(rtn) has been called before calling this function.
 *
 * @param rtn A function unwinding thread's stack.
 * @param callback A callback function that will be called when the unwinding
 *   of the stack is complete and the new value of the stack pointer is known.
 */
VOID instrumentUnwindFunction(RTN rtn, UNWINDFUNPTR callback)
{
  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
  { // Find the instruction where the new value of the stack pointer is known
    if (INS_RegWContain(ins, REG_STACK_PTR))
    { // We are interested only in instructions working with the stack pointer
      if (INS_IsMov(ins))
      { // Usually the new value is computed and then moved to the SP register
        LOG("      [X] Found instruction completing the stack unwinding at "
          + *retrieveInstruction(indexInstruction(ins)) + "\n");

        // Extract the new value of SP and pass it to the callback function
        INS_InsertCall(
          ins, IPOINT_AFTER, (AFUNPTR)callback,
          IARG_FAST_ANALYSIS_CALL,
          IARG_THREAD_ID,
          IARG_REG_VALUE, REG_STACK_PTR,
          IARG_END);
      }
      else
      { // Log the skipped instructions for debug purposes
        LOG("      [ ] Ignoring instruction modifying the stack pointer at "
          + *retrieveInstruction(indexInstruction(ins)) + "\n");
      }
    }
  }
}

/** End of file unwind.cpp **/
