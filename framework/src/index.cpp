/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of functions for accessing various indexes.
 *
 * A file containing implementation of functions for accessing various indexes.
 *
 * @file      index.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-07-18
 * @version   0.5.2.1
 */

#include "index.h"

#include <assert.h>

#include <string>
#include <vector>

#include "defs.h"

#include "utils/lockobj.hpp"
#include "utils/scopedlock.hpp"

/**
 * @brief An index which does not check for duplicate values.
 *
 * Stores the indexed values in a vector. Does not check if newly indexed values
 *   are already present in the index to speed up the index process.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-09-07
 * @date      Last Update 2016-04-28
 * @version   0.2.2
 */
template < class ValueType >
class FastIndexImpl : public RWLockableObject
{
  protected: // Type definitions
    typedef std::vector< ValueType > Index;
    typedef typename Index::value_type value_type;
    typedef typename Index::const_reference const_reference;
  protected: // Internal variables
    Index m_index; //!< A container holding the indexed values.
  public: // Inline generated methods
    /**
     * Stores an object in the index.
     *
     * @param obj A reference to the object to be stored in the index.
     * @return The position of the object in the index.
     */
    inline
    index_t indexObject(const_reference obj)
    {
      // For now, assume we can index objects concurrently
      ScopedWriteLock writelock(this->m_lock);

      // Do not check for duplicates, just index the value
      m_index.push_back(obj);

      // Index of the value is the position of the value in the vector
      return m_index.size() - 1;
    }

    /**
     * Retrieves an object from the index.
     *
     * @param idx The position of the object in the index.
     * @return A reference to the object stored in the index at the specified
     *   position.
     */
    inline
    const_reference retrieveObject(index_t idx)
    {
      // PIN uses non-thread-safe version of stdlib, ensure thread-safety here
      ScopedReadLock readlock(this->m_lock);

      // Only indexes returned by the indexObject method should be passed here
      assert(idx < m_index.size());

      // Return a reference to the object stored at the specified index
      return m_index[idx];
    }
};

/**
 * @brief A generic index supporting any type of values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-28
 * @date      Last Update 2016-04-28
 * @version   0.1
 */
template < class ValueType >
class FastIndex : public FastIndexImpl< ValueType > {};

/**
 * @brief An index tailored for indexing string values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-28
 * @date      Last Update 2016-04-28
 * @version   0.1
 */
template <>
class FastIndex< std::string > : public FastIndexImpl< std::string >
{
  public:
    /**
     * Retrieves a string from the index.
     *
     * @note This method returns a @b copy of the indexed string instead of a
     *   reference to it. The reason for that is to ensure thread safety that
     *   may be broken by the compiler and its CoW (and other) optimisations.
     *   These optimisations do things like secret sharing (sharing substring
     *   that is part of other string) which are not thread-safe and may lead
     *   to program crashes and other errors when used in concurrent setting.
     *
     * @param idx The position of the string in the index.
     * @return A copy of the string stored in the index at the specified
     *   position.
     */
    inline
    FastIndexImpl< std::string >::value_type retrieveObject(index_t idx)
    {
      // PIN uses non-thread-safe version of stdlib, ensure thread-safety here
      ScopedReadLock readlock(this->m_lock);

      // Only indexes returned by the indexObject method should be passed here
      assert(idx < m_index.size());

      // Returning a C string forces compiler not to use CoW optimisations
      return m_index[idx].c_str();
    }
};

namespace
{ // Static global variables (usable only within this module)
  FastIndex< const IMAGE* > g_imageIndex;
  FastIndex< const FUNCTION* > g_functionIndex;
  FastIndex< const CALL* > g_callIndex;
  FastIndex< const INSTRUCTION* > g_instructionIndex;
  FastIndex< const LOCATION* > g_locationIndex;

  std::string g_emptyString = ""; // Used for referencing unknown values
}

/**
 * Gets an offset of an instruction in its image.
 *
 * @param ins An object representing the instruction.
 * @return The offset of the instruction in its image or @c 0 if the offset
 *   cannot be determined.
 */
ADDRINT getOffset(const INS ins)
{
  // Helper variables
  IMG image;

  if (RTN_Valid(INS_Rtn(ins)))
  { // Find the image through the function containing the instruction
    image = SEC_Img(RTN_Sec(INS_Rtn(ins)));
  }
  else
  { // The instruction is not part of any function, find image by address
    image = IMG_FindByAddress(INS_Address(ins));
  }

  // Could not find the image containing the instruction, unknown offset
  if (!IMG_Valid(image)) return 0;

  // Offset = [instruction address] - [address where the image was loaded]
  return INS_Address(ins) - IMG_LowAddress(image);
}

/**
 * Stores information about an image in the image index.
 *
 * @param image A structure containing information about the image.
 * @return A position in the image index where the information about the image
 *   were stored.
 */
index_t indexImage(const IMAGE* image)
{
  return g_imageIndex.indexObject(image);
}

/**
 * Stores information about a function in the function index.
 *
 * @param function A structure containing information about the function.
 * @return A position in the function index where the information about the
 *   function were stored.
 */
index_t indexFunction(const FUNCTION* function)
{
  return g_functionIndex.indexObject(function);
}

/**
 * Stores information about a call in the call index.
 *
 * @param call A structure containing information about the call.
 * @return A position in the call index where the information about the call
 *   were stored.
 */
index_t indexCall(const CALL* call)
{
  return g_callIndex.indexObject(call);
}

/**
 * Stores information about an instruction in the instruction index.
 *
 * @param instruction A structure containing information about the instruction.
 * @return A position in the instruction index where the information about the
 *   instruction were stored.
 */
index_t indexInstruction(const INSTRUCTION* instruction)
{
  return g_instructionIndex.indexObject(instruction);
}

/**
 * Stores information about a source code location in the location index.
 *
 * @param location A structure containing information about the source code
 *   location.
 * @return A position in the location index where the information about the
 *   source code location were stored.
 */
index_t indexLocation(const LOCATION* location)
{
  return g_locationIndex.indexObject(location);
}

/**
 * Stores information about an image in the image index.
 *
 * @param img An object representing the image.
 * @return A position in the image index where the information about the image
 *   were stored.
 */
index_t indexImage(const IMG img)
{
  // At index 0 should be an entry representing an unknown image
  if (!IMG_Valid(img)) return 0;

  return indexImage(new IMAGE(
    IMG_Name(img)
    ));
}

/**
 * Stores information about a function in the function index.
 *
 * @param rtn An object representing the function.
 * @return A position in the function index where the information about the
 *   function were stored.
 */
index_t indexFunction(const RTN rtn)
{
  // At index 0 should be an entry representing an unknown function
  if (!RTN_Valid(rtn)) return 0;

  return indexFunction(new FUNCTION(
    PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY),
    RTN_Name(rtn),
    indexImage(SEC_Img(RTN_Sec(rtn)))
    ));
}

/**
 * Stores information about a call in the call index.
 *
 * @param ins An object representing the call.
 * @return A position in the call index where the information about the call
 *   were stored.
 */
index_t indexCall(const INS ins)
{
  return indexCall(new CALL(
    getOffset(ins),
    indexFunction(INS_Rtn(ins)),
    indexLocation(ins)
    ));
}

/**
 * Stores information about an instruction in the instruction index.
 *
 * @param ins An object representing the instruction.
 * @return A position in the instruction index where the information about the
 *   instruction were stored.
 */
index_t indexInstruction(const INS ins)
{
  return indexInstruction(new INSTRUCTION(
    getOffset(ins),
    indexFunction(INS_Rtn(ins)),
    indexLocation(ins)
    ));
}

/**
 * Stores information about a source code location in the location index.
 *
 * @param ins An object representing any instruction which corresponds to the
 *   source code location.
 * @return A position in the location index where the information about the
 *   source code location were stored.
 */
index_t indexLocation(const INS ins)
{
  LOCATION* location = new LOCATION();

  PIN_GetSourceLocation(INS_Address(ins),
    NULL, &location->line, &location->file);

  return indexLocation(location);
}

/**
 * Retrieves information about an image from the image index.
 *
 * @param idx A position in the image index where the information about the
 *   image are stored.
 * @return A structure containing information about the image.
 */
const IMAGE* retrieveImage(index_t idx)
{
  return g_imageIndex.retrieveObject(idx);
}

/**
 * Retrieves information about a function from the image index.
 *
 * @param idx A position in the function index where the information about the
 *   function are stored.
 * @return A structure containing information about the function.
 */
const FUNCTION* retrieveFunction(index_t idx)
{
  return g_functionIndex.retrieveObject(idx);
}

/**
 * Retrieves information about a call from the image index.
 *
 * @param idx A position in the call index where the information about the call
 *   are stored.
 * @return A structure containing information about the call.
 */
const CALL* retrieveCall(index_t idx)
{
  return g_callIndex.retrieveObject(idx);
}

/**
 * Retrieves information about an instruction from the image index.
 *
 * @param idx A position in the instruction index where the information about
 *   the instruction are stored.
 * @return A structure containing information about the instruction.
 */
const INSTRUCTION* retrieveInstruction(index_t idx)
{
  return g_instructionIndex.retrieveObject(idx);
}

/**
 * Retrieves information about a source code location from the location index.
 *
 * @param idx A position in the location index where the information about the
 *   source code location are stored.
 * @return A structure containing information about the source code location.
 */
const LOCATION* retrieveLocation(index_t idx)
{
  return g_locationIndex.retrieveObject(idx);
}

/**
 * Initialises the indexes.
 */
VOID setupIndexModule()
{
  // First entry in each index represents a valid, yet unknown, information
  index_t unknownImageIdx = indexImage(new IMAGE(g_emptyString));
  index_t unknownFunctionIdx = indexFunction(new FUNCTION(
    g_emptyString, g_emptyString, unknownImageIdx));
  index_t unknownLocationIdx = indexLocation(new LOCATION(g_emptyString, 0));
  index_t unknownCallIdx = indexCall(new CALL(
    0, unknownFunctionIdx, unknownLocationIdx));
  index_t unknownInstructionIdx = indexInstruction(new INSTRUCTION(
    0, unknownFunctionIdx, unknownLocationIdx));

  // Variables used only in assertion checks
  ASSERT_VARIABLE(unknownCallIdx);
  ASSERT_VARIABLE(unknownInstructionIdx);

  // Check if the entries are the first ones inserted into the indexes
  assert(unknownImageIdx == 0);
  assert(unknownFunctionIdx == 0);
  assert(unknownLocationIdx == 0);
  assert(unknownCallIdx == 0);
  assert(unknownInstructionIdx == 0);
}

/** End of file index.cpp **/
