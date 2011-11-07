/**
 * @brief A file implementation definitions of type-to-type mapping class.
 *
 * A file implementation definitions of classes for mapping objects of one type
 *   to objects of another type.
 *
 * @file      mapper.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-03
 * @date      Last Update 2011-11-07
 * @version   0.1
 */

#include "mapper.h"

// Initialisation of the singleton instance
FuncArgMapperFactory* FuncArgMapperFactory::ms_instance = NULL;

/**
 * Gets a singleton instance.
 *
 * @note If no singleton instance exist, the method will create one.
 *
 * @return The singleton instance.
 */
FuncArgMapperFactory* FuncArgMapperFactory::Get()
{
  if (ms_instance == NULL)
  { // No singleton instance yet, create one
    ms_instance = new FuncArgMapperFactory();
  }

  return ms_instance;
}

/**
 * Gets a function argument mapper.
 *
 * @param name A name identifying the function argument mapper.
 * @return A function argument mapper or @em NULL if no function argument mapper
 *   with the specified name is registered.
 */
FuncArgMapper* FuncArgMapperFactory::getMapper(std::string name)
{
  // Try to find a function argument mapper with the specified name
  FuncArgMapperMap::iterator it = m_registeredMappers.find(name);

  if (it != m_registeredMappers.end())
  { // Function argument mapper found, return it
    return it->second;
  }

  // No function argument mapper found
  return NULL;
}

/**
 * Registers a function argument mapper.
 *
 * @warning If a function argument mapper with the specified name already exist,
 *   it will be overwritten by the specified function argument mapper.
 *
 * @param name A name identifying the function argument mapper.
 * @param mapper A function argument mapper.
 */
void FuncArgMapperFactory::registerMapper(std::string name,
  FuncArgMapper* mapper)
{
  m_registeredMappers[name] = mapper;
}

/**
 * Constructs an AddressLockMapper object.
 */
AddressFuncArgMapper::AddressFuncArgMapper() : m_lastIndex(0)
{
}

/**
 * Maps addresses to unique IDs.
 *
 * @param addr An address.
 * @return A unique ID assigned to the specified address.
 */
UINT32 AddressFuncArgMapper::map(ADDRINT* addr)
{
  // Check if a lock on the specified address do not already have a mapping
  std::map< ADDRINT, UINT32 >::iterator it = m_indexMap.find(*addr);

  if (it != m_indexMap.end())
  { // Lock on the specified address already have a corresponding lock object
    return it->second;
  }

  // Return the newly created lock object for the lock on the specified address
  return m_indexMap[*addr] = ++m_lastIndex;
}

/** End of file mapper.cpp **/
