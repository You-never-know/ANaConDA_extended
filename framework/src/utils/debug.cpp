/*
 * Copyright (C) 2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains helper functions used for debugging the framework.
 *
 * A file containing the implementation of helper functions used for debugging
 *   the framework.
 *
 * @file      debug.cp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-05-09
 * @date      Last Update 2019-06-04
 * @version   0.1
 */

#include "debug.h"

/**
 * Prints a detailed information about the memory access whose assertion failed.
 *
 * @param message A message describing the assertion that failed.
 * @param insAddr An address of the instruction.
 * @param rtnAddr An address of the function containing the instruction.
 */
void memoryAccessAssertionFailed(const char* message, ADDRINT insAddr,
  ADDRINT rtnAddr)
{
  // The code below may be executed from a PIN analysis function
  PIN_LockClient();

  // Get the function containing the instruction
  RTN rtn = RTN_FindByAddress(rtnAddr);

  // We cannot translate instruction address to instruction directly
  RTN_Open(rtn);

  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
  { // So we must go through all instructions in the function
    if (INS_Address(ins) == insAddr)
    { // And find the instruction with the address specified
      CONSOLE(std::string(message)
        + "\n  instruction: " + INS_Disassemble(ins) + " @ " + hexstr(insAddr)
        + "\n  in function: " + RTN_Name(rtn) + " @ " + hexstr(rtnAddr)
        + "\n");

      break;
    }
  }

  RTN_Close(rtn);

  PIN_UnlockClient();
}

/** End of file debug.cpp **/
