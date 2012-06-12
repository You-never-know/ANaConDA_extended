/**
 * @brief Contains implementation of a concurrent map guarded by an R/W lock.
 *
 * A file containing implementation of a class providing thread-safe access to
 *   a map. The access is guarded by an R/W (Reader/Writer) lock.
 *
 * @file      rwmap.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-02
 * @date      Last Update 2012-06-12
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__PIN__RWMAP_HPP__
  #define __PINTOOL_ANACONDA__PIN__RWMAP_HPP__

#include <map>

#include "pin.H"

/**
 * @brief A class representing a concurrent map guarded by an R/W lock.
 *
 * Provides thread-safe access to a map. The access is guarded by an R/W
 *   (Reader/Writer) lock.
 *
 * @warning The implementation assumes that the map is used in a way that the
 *   values are never changed after inserted!
 *
 * @tparam KEY A type of data representing map's keys.
 * @tparam VALUE A type of data representing map's values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-02
 * @date      Last Update 2012-06-12
 * @version   0.1
 */
template< typename KEY, typename VALUE >
class RWMap
{
  public: // Definition of underlying map
    typedef std::map< KEY, VALUE > Map;
  private: // Internal variables
    Map m_map; //!< An underlying map containing key/value pairs.
    PIN_RWMUTEX m_lock; //!< A lock guarding R/W access to the map.
    VALUE m_defaultValue; //!< A default value returned when a key is not found.
  public: // Constructors
    RWMap() { PIN_RWMutexInit(&m_lock); }
    RWMap(VALUE dv) : m_defaultValue(dv) { PIN_RWMutexInit(&m_lock); }
  public: // Destructors
    ~RWMap() { PIN_RWMutexFini(&m_lock); }
  public: // Inline generated methods
    /**
     * Gets a value associated with a specific key.
     *
     * @param key A key to be searched for.
     * @return The value associated with the specified key or the default value
     *   specified during object construction if the key was not found.
     */
    const VALUE& get(const KEY& key)
    { // Finding requires only read access to the underlying map
      PIN_RWMutexReadLock(&m_lock);

      // Check if the key is present in the underlying map
      Map::iterator it = m_map.find(key);

      // Insert do not invalidate the iterator, delete is not allowed, so it is
      // safe to release the lock now and continue using the iterator
      PIN_RWMutexUnlock(&m_lock);

      if (it != m_map.end())
      { // Key found, return the associated value
        return it->second;
      }

      // Key not found, return the default value
      return m_defaultValue;
    }

    /**
     * Inserts a specific key with a value associated with it.
     *
     * @param key A key to be inserted.
     * @param value A value associated with the key.
     */
    void insert(const KEY& key, const VALUE& value)
    { // Inserting a key/value pair must be exclusive
      PIN_RWMutexWriteLock(&m_lock);

      // Insert the key/value pair into the underlying map
      m_map.insert(Map::value_type(key, value));

      // Key/value pair inserted, allow other threads to access the map
      PIN_RWMutexUnlock(&m_lock);
    }
};

#endif /* __PINTOOL_ANACONDA__PIN__RWMAP_HPP__ */

/** End of file rwmap.hpp **/
