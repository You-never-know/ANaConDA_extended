/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of functions for working with backtraces.
 *
 * A file containing implementation of various helper functions for working with
 *   backtraces.
 *
 * @file      backtrace.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-11-26
 * @date      Last Update 2013-05-30
 * @version   0.2.0.2
 */

#ifndef __PINTOOL_ANACONDA__UTILS__BACKTRACE_HPP__
  #define __PINTOOL_ANACONDA__UTILS__BACKTRACE_HPP__

#include "pin.H"

#include "../settings.h"

/**
 * @brief An enumeration of types of function implementations.
 */
typedef enum FunctionImplementation_e
{
  /**
   * @brief Best performance, but \b cannot be used in PIN analysis functions.
   */
  FI_BARE   = 0x0,
  /**
   * @brief Worse performance, but \b can be used in PIN analysis functions.
   */
  FI_LOCKED = 0x1
} FunctionImplementation;

/**
 * Creates a location for an instruction on a specific address which will be
 *   used in a backtrace.
 *
 * @tparam BV Determines how detailed the location will be.
 * @tparam FI Determines the implementation of the function.
 *
 * @param ins An instruction.
 * @return A location of the instruction.
 */
template < BacktraceVerbosity BV, FunctionImplementation FI = FI_BARE >
inline
std::string makeBacktraceLocation(ADDRINT insAddr)
{
  // Helper variables
  std::string location;
  INT32 line;

  // Calling GetSourceLocation in PIN analysis function requires a client lock
  if (FI & FI_LOCKED) PIN_LockClient();

  // Gather basic information (the location of the instruction) first
  PIN_GetSourceLocation(insAddr, NULL, &line, &location);

  if (FI & FI_LOCKED) PIN_UnlockClient();

  // Location might not be available if no debug information is present
  location += location.empty() ? "<unknown>" : ":" + decstr(line);

  if (BV & (BV_DETAILED | BV_MAXIMAL))
  { // Calling FindByAddress in PIN analysis function requires a client lock
    if (FI & FI_LOCKED) PIN_LockClient();

    // For detailed information, we need the name of the image and function
    RTN rtn = RTN_FindByAddress(insAddr);

    if (FI & FI_LOCKED) PIN_UnlockClient();

    // Not all instructions are in a function, every function is in some image
    location = (RTN_Valid(rtn) ? IMG_Name(SEC_Img(RTN_Sec(rtn))) + "!"
      + RTN_Name(rtn) : "<unknown>!<unknown>") + "(" + location + ")";

    if (BV & BV_MAXIMAL)
    { // To locate an instruction within a disassembled code we need its offset
      location += " [instruction at offset " + (RTN_Valid(rtn)
        ? hexstr(insAddr - IMG_LowAddress(SEC_Img(RTN_Sec(rtn))))
        : "<unknown>") + "]";
    }
  }

  // Location successfully created
  return location;
}

/**
 * Creates a location for an instruction which will be used in a backtrace.
 *
 * @tparam BV Determines how detailed the location will be.
 * @tparam FI Determines the implementation of the function.
 *
 * @param ins An instruction.
 * @return A location of the instruction.
 */
template < BacktraceVerbosity BV, FunctionImplementation FI = FI_BARE >
inline
std::string makeBacktraceLocation(INS ins)
{
  return makeBacktraceLocation< BV, FI >(INS_Address(ins));
}

#endif /* __PINTOOL_ANACONDA__UTILS__BACKTRACE_HPP__ */

/** End of file backtrace.hpp **/
