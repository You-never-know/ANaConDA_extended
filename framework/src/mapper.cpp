/**
 * @brief Contains implementation of type-to-type mapping classes.
 *
 * A file containing implementation of classes for mapping objects of one type
 *   to objects of another type.
 *
 * @file      mapper.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-03
 * @date      Last Update 2012-01-27
 * @version   0.1.1
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
  PIN_RWMutexInit(&m_indexMutex);
}

/**
 * Destroys a AddressFuncArgMapper object.
 */
AddressFuncArgMapper::~AddressFuncArgMapper()
{
  PIN_RWMutexFini(&m_indexMutex);
}

/**
 * Maps addresses to unique IDs.
 *
 * @param addr An address.
 * @return A unique ID assigned to the specified address.
 */
const UINT32 AddressFuncArgMapper::map(ADDRINT* addr)
{
  // Guarding find() is sufficient, insert() do not invalidate the iterator
  PIN_RWMutexReadLock(&m_indexMutex);

  // Check if a lock on the specified address do not already have a mapping
  IndexMap::iterator it = m_indexMap.find(*addr);

  // No need to guard accesses to the map's value anymore, it is a constant
  PIN_RWMutexUnlock(&m_indexMutex);

  if (it != m_indexMap.end())
  { // Lock on the specified address already have a corresponding lock object
    return it->second;
  }

  // Writing to the index map and updating the index counter must be exclusive
  PIN_RWMutexWriteLock(&m_indexMutex);

  // Reuse the returned iterator to safely return the value (no need to guard)
  it = m_indexMap.insert(IndexMap::value_type(*addr, ++m_lastIndex)).first;

  // Iterator must be valid, value is a constant, safe to enable reading again
  PIN_RWMutexUnlock(&m_indexMutex);

  // Return the value representing an object on the specified address
  return it->second;
}

/** End of file mapper.cpp **/
