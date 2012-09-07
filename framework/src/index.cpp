/**
 * @brief Contains implementation of functions for accessing various indexes.
 *
 * A file containing implementation of functions for accessing various indexes.
 *
 * @file      index.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2012-09-07
 * @version   0.1
 */

#include "index.h"

#include <assert.h>

#include <string>
#include <vector>

/**
 * @brief An index which does not check for duplicate values.
 *
 * Stores the indexed values in a vector. Does not check if newly indexed values
 *   are already present in the index to speed up the index process.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-09-07
 * @date      Last Update 2012-09-07
 * @version   0.1
 */
template < class ValueType >
class FastIndex
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
    const ValueType& retrieveObject(index_t idx)
    {
      // Only indexes returned by the indexObject method should be passed here
      assert(idx < m_index.size());

      // So the index should always be valid here
      return m_index[idx];
    }
};

// Type definitions
typedef FastIndex< std::string > FastStringIndex;

namespace
{ // Static global variables (usable only within this module)
  FastStringIndex g_imageIndex;
  FastStringIndex g_functionIndex;
  FastStringIndex g_callIndex;
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
 * Retrieves a name of an image from the image index.
 *
 * @param idx The position of an image stored in the image index.
 * @return The name of the image stored at the specified position in the image
 *   index.
 */
const std::string& retrieveImage(index_t idx)
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
const std::string& retrieveFunction(index_t idx)
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
const std::string& retrieveCall(index_t idx)
{
  return g_callIndex.retrieveObject(idx);
}

/** End of file index.cpp **/
