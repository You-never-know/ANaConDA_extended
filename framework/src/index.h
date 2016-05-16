/**
 * @brief Contains definitions of functions for accessing various indexes.
 *
 * A file containing definitions of functions for accessing various indexes.
 *
 * @file      index.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-05-16
 * @version   0.5.1
 */

#ifndef __PINTOOL_ANACONDA__INDEX_H__
  #define __PINTOOL_ANACONDA__INDEX_H__

#include "pin.H"

#include "types.h"

// Definitions of functions for indexing various (framework) data
index_t indexImage(const IMAGE* image);
index_t indexFunction(const FUNCTION* function);
index_t indexCall(const CALL* call);
index_t indexInstruction(const INSTRUCTION* instruction);
index_t indexLocation(const LOCATION* location);

// Definitions of functions for indexing various (Intel PIN) data
index_t indexImage(const IMG img);
index_t indexFunction(const RTN rtn);
index_t indexCall(const INS ins);
index_t indexInstruction(const INS ins);
index_t indexLocation(const INS ins);

// Definitions of functions for accessing indexed data
const IMAGE* retrieveImage(index_t idx);
const FUNCTION* retrieveFunction(index_t idx);
const CALL* retrieveCall(index_t idx);
const INSTRUCTION* retrieveInstruction(index_t idx);
const LOCATION* retrieveLocation(index_t idx);

// Definitions of helper functions
VOID setupIndexModule();

// Definitions of inline functions for printing indexed data
/**
 * Concatenates a string with a structure representing a source code location.
 *
 * @param s A string.
 * @param loc A structure representing a source code location.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em loc.
 */
inline
std::string operator+(const std::string& s, const LOCATION& loc)
{
  return s + loc.file + ":" + decstr(loc.line);
}

/**
 * Concatenates a structure representing a source code location with a string.
 *
 * @param loc A structure representing a source code location.
 * @param s A string.
 * @return A new string with a value of a string representation of @em loc
 *   followed by @em s.
 */
inline
std::string operator+(const LOCATION& loc, const std::string& s)
{
  return loc.file + ":" + decstr(loc.line) + s;
}

/**
 * Concatenates a string with a structure representing an image.
 *
 * @param s A string.
 * @param img A structure representing an image.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em img.
 */
inline
std::string operator+(const std::string& s, const IMAGE& img)
{
  return s + img.path;
}

/**
 * Concatenates a structure representing an image with a string.
 *
 * @param img A structure representing an image.
 * @param s A string.
 * @return A new string with a value of a string representation of @em img
 *   followed by @em s.
 */
inline
std::string operator+(const IMAGE& img, const std::string& s)
{
  return img.path + s;
}

/**
 * Concatenates a string with a structure representing a function.
 *
 * @param s A string.
 * @param fun A structure representing a function.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em fun.
 */
inline
std::string operator+(const std::string& s, const FUNCTION& fun)
{
  return s + *retrieveImage(fun.image) + "!" + fun.name;
}

/**
 * Concatenates a structure representing a function with a string.
 *
 * @param fun A structure representing a function.
 * @param s A string.
 * @return A new string with a value of a string representation of @em fun
 *   followed by @em s.
 */
inline
std::string operator+(const FUNCTION& fun, const std::string& s)
{
  return *retrieveImage(fun.image) + "!" + fun.name + s;
}

/**
 * Concatenates a string with a structure representing an instruction.
 *
 * @param s A string.
 * @param ins A structure representing a instruction.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em ins.
 */
inline
std::string operator+(const std::string& s, const INSTRUCTION& ins)
{
  return s + *retrieveFunction(ins.function) + ":" + hexstr(ins.offset) + " ("
    + *retrieveLocation(ins.location) + ")";
}

/**
 * Concatenates a structure representing an instruction with a string.
 *
 * @param ins A structure representing a instruction.
 * @param s A string.
 * @return A new string with a value of a string representation of @em ins
 *   followed by @em s.
 */
inline
std::string operator+(const INSTRUCTION& ins, const std::string& s)
{
  return *retrieveFunction(ins.function) + ":" + hexstr(ins.offset) + " ("
    + *retrieveLocation(ins.location) + ")" + s;
}

#endif /* __PINTOOL_ANACONDA__INDEX_H__ */

/** End of file index.h **/
