/**
 * @brief Contains implementation of functions for accessing various indexes.
 *
 * A file containing implementation of functions for accessing various indexes.
 *
 * @file      index.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-03-22
 * @version   0.2
 */

#include "index.h"

#include <assert.h>

#include <string>
#include <vector>

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
 * @date      Last Update 2016-03-22
 * @version   0.2
 */
template < class ValueType >
class FastIndex : public RWLockableObject
{
  private: // Internal variables
    std::vector< ValueType > m_index; //!< A vector to store the indexed values.
  public: // Inline generated methods
    /**
     * Stores an object in the index.
     *
     * @param obj An object to be stored in the index.
     * @return The position of the object in the index.
     */
    inline
    index_t indexObject(const ValueType& obj)
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
     * @param idx The position of an object in the index.
     * @return The object stored in the index at the specified position.
     */
    inline
    ValueType retrieveObject(index_t idx)
    {
      // PIN uses non-thread-safe version of stdlib, ensure thread-safety here
      ScopedReadLock readlock(this->m_lock);

      // Only indexes returned by the indexObject method should be passed here
      assert(idx < m_index.size());

      // So the index should always be valid here, by returning a C string we
      // force the compiler not to use any CoW optimisations used with string
      return m_index[idx].c_str();
    }
};

// Type definitions
typedef FastIndex< std::string > FastStringIndex;

namespace
{ // Static global variables (usable only within this module)
  FastStringIndex g_imageIndex;
  FastStringIndex g_functionIndex;
  FastStringIndex g_callIndex;
  FastStringIndex g_instructionIndex;
}

/**
 * Stores a name of an image in the image index.
 *
 * @param name A name of an image.
 * @return The position of the image in the image index.
 */
index_t indexImage(const std::string& name)
{
  return g_imageIndex.indexObject(name);
}

/**
 * Stores a name of a function in the function index.
 *
 * @param name A name of a function.
 * @return The position of the function in the function index.
 */
index_t indexFunction(const std::string& name)
{
  return g_functionIndex.indexObject(name);
}

/**
 * Stores a description of a call in the call index.
 *
 * @param desc A description of a call.
 * @return The position of the call in the call index.
 */
index_t indexCall(const std::string& desc)
{
  return g_callIndex.indexObject(desc);
}

/**
 * Stores a disassembly of an instruction in the instruction index.
 *
 * @param dasm A disassembly of an instruction.
 * @return The position of the instruction in the instruction index.
 */
index_t indexInstruction(const std::string& dasm)
{
  return g_instructionIndex.indexObject(dasm);
}

/**
 * Retrieves a name of an image from the image index.
 *
 * @param idx The position of an image stored in the image index.
 * @return The name of the image stored at the specified position in the image
 *   index.
 */
std::string retrieveImage(index_t idx)
{
  return g_imageIndex.retrieveObject(idx);
}

/**
 * Retrieves a name of a function from the function index.
 *
 * @param idx The position of a function stored in the function index.
 * @return The name of the function stored at the specified position in the
 *   function index.
 */
std::string retrieveFunction(index_t idx)
{
  return g_functionIndex.retrieveObject(idx);
}

/**
 * Retrieves a description of a call from the call index.
 *
 * @param idx The position of a call stored in the call index.
 * @return The description of the call stored at the specified position in the
 *   call index.
 */
std::string retrieveCall(index_t idx)
{
  return g_callIndex.retrieveObject(idx);
}

/**
 * Retrieves a disassembly of an instruction from the instruction index.
 *
 * @param idx The position of an instruction stored in the instruction index.
 * @return The disassembly of the instruction stored at the specified position
 *   in the instruction index.
 */
std::string retrieveInstruction(index_t idx)
{
  return g_instructionIndex.retrieveObject(idx);
}

/** End of file index.cpp **/
