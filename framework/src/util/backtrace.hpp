/**
 * @brief Contains implementation of functions for working with backtraces.
 *
 * A file containing implementation of various helper functions for working with
 *   backtraces.
 *
 * @file      backtrace.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-11-26
 * @date      Last Update 2012-11-26
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__PIN__BACKTRACE_HPP__
  #define __PINTOOL_ANACONDA__PIN__BACKTRACE_HPP__

#include "pin.H"

/**
 * Creates a location for an instruction which will be used in a backtrace.
 *
 * @param ins An instruction.
 * @return A location of the instruction.
 */
template < BacktraceVerbosity BTV >
inline
std::string makeBacktraceLocation(INS ins)
{
  // Helper variables
  std::string location;
  INT32 line;

  // Gather basic information (the location of the instruction) first
  PIN_GetSourceLocation(INS_Address(ins), NULL, &line, &location);

  // Location might not be available if no debug information is present
  location += location.empty() ? "<unknown>" : ":" + decstr(line);

  if (BTV & (DETAILED | DEBUG))
  { // For detailed information, we need the name of the image and function
    RTN rtn = INS_Rtn(ins);

    // Not all instructions are in a function, every function is in some image
    location = (RTN_Valid(rtn) ? IMG_Name(SEC_Img(RTN_Sec(rtn))) + "!"
      + RTN_Name(rtn) : "<unknown>!<unknown>") + "(" + location + ")";

    if (BTV & DEBUG)
    { // To locate an instruction within a disassembled code we need its offset
      location += " [instruction at offset " + (RTN_Valid(rtn)
        ? hexstr(INS_Address(ins) - IMG_LowAddress(SEC_Img(RTN_Sec(rtn))))
        : "<unknown>") + "]";
    }
  }

  // Location successfully created
  return location;
}

#endif /* __PINTOOL_ANACONDA__PIN__BACKTRACE_HPP__ */

/** End of file backtrace.hpp **/
