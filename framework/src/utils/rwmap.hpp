/**
 * @brief Contains implementations of concurrent maps guarded by an R/W lock.
 *
 * A file containing various implementations of classes providing thread-safe
 *   access to a map. The access is guarded by a read/write (R/W) lock. There
 *   are various implementations, each having its own restrictions imposed by
 *   the concrete implementation used. The goal is to provide implementations
 *   tailored for specific situations and let the developer to chose the most
 *   suitable one. More restricted implementations are fast, however, they do
 *   allow some operations to be performed on the map. On the other hand, the
 *   less restricted implementations allow a wider variety of operations that
 *   can be performed on the map, however, they are much slower.
 *
 * @file      rwmap.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-02
 * @date      Last Update 2016-03-22
 * @version   0.3.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__RWMAP_HPP__
  #define __PINTOOL_ANACONDA__UTILS__RWMAP_HPP__

#include <assert.h>

#include <map>

#include "lockobj.hpp"
#include "scopedlock.hpp"

/**
 * @brief An concurrent R/W map which does not allow any updates.
 *
 * An effective implementation of an R/W map which requires that the values are
 *   never changed after inserted. This map allows one to insert a new value to
 *   the map and access this value by a reference. Deleting and updating is not
 *   allowed as it may invalidate the references returned by the implementation.
 *
 * The access to the map is guarded by a read/write lock which ensures that all
 *   operations are thread-safe. This lock ensures that nobody is searching for
 *   values when new values are being inserted into the map.
 *
 * @warning This map requires that only a single value can be inserted for each
 *   unique key. After the value is inserted, it can never be changed. If a new
 *   value for an existing key is inserted, it will raise an assertion error!
 *
 * @note Obtained references to values in the map are always valid as values in
 *   the map cannot change after they are (safely) inserted to the map.
 *
 * @tparam KEY A type of data representing mapped keys.
 * @tparam VALUE A type of data representing mapped values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-02
 * @date      Last Update 2016-03-18
 * @version   0.2
 */
template< typename KEY, typename VALUE >
class ImmutableRWMap : public RWLockableObject
{
  public: // Definition of underlying map
    typedef std::map< KEY, VALUE > Map;
  private: // Internal variables
    Map m_map; //!< An underlying map containing key/value pairs.
    VALUE m_defaultValue; //!< A default value returned when a key is not found.
  public: // Constructors
    ImmutableRWMap(const VALUE& dv) : m_defaultValue(dv) {}
  public: // Inline generated methods
    /**
     * Gets a reference to a value associated with a specific key.
     *
     * @note This reference is always valid as the map does not allow changing
     *   the value and thus invalidating the reference in the process.
     *
     * @param key A key to be searched for.
     * @return The reference to the value associated with the specified key or
     *   to the default value specified during object construction if the key
     *   was not found.
     */
    const VALUE& get(const KEY& key)
    { // Finding requires only read access to the underlying map
      this->readlock();

      // Check if the key is present in the underlying map
      typename Map::iterator it = m_map.find(key);

      // Insert do not invalidate the iterator, delete is not allowed, so it is
      // safe to release the lock now and continue using the iterator
      this->unlock();

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
     * @warning Inserting a value for a key that already exists in the map
     *   raises an assertion error!
     *
     * @param key A key to be inserted.
     * @param value A value associated with the key.
     */
    void insert(const KEY& key, const VALUE& value)
    { // Inserting a key/value pair must be exclusive
      this->writelock();

      // It is not allowed to update values for existing keys
      assert(m_map.count(key) == 0);

      // Insert the key/value pair into the underlying map
      m_map.insert(typename Map::value_type(key, value));

      // Key/value pair inserted, allow other threads to access the map
      this->unlock();
    }
};

/**
 * @brief An concurrent R/W map which allows (unsafe) updates.
 *
 * An effective implementation of an R/W map which allows (unsafe) updates. The
 *   map allows one to update existing values (or insert new values) and access
 *   these values by a reference, however, the user itself must ensure that the
 *   values are not updated when other threads are still using the old values!
 *
 * The access to the map is guarded by a read/write lock which ensures that all
 *   operations are thread-safe. This lock ensures that nobody is searching for
 *   values when they are being updated (or inserted). It does not check if any
 *   thread is still using the old values using a reference returned before!
 *
 * @warning Obtained references to values in the map may be invalidated if the
 *   values are updated! It is up to the developer to ensure that the values
 *   are updated only when not used by the other threads!
 *
 * @tparam KEY A type of data representing mapped keys.
 * @tparam VALUE A type of data representing mapped values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-03-18
 * @date      Last Update 2016-03-22
 * @version   0.1.1
 */
template< typename KEY, typename VALUE >
class UnsafeRWMap : public RWLockableObject
{
  public: // Definition of underlying map
    typedef std::map< KEY, VALUE > Map;
  private: // Internal variables
    Map m_map; //!< An underlying map containing key/value pairs.
    VALUE m_defaultValue; //!< A default value returned when a key is not found.
  public: // Constructors
    UnsafeRWMap(const VALUE& dv) : m_defaultValue(dv) {}
  public: // Inline generated methods
    /**
     * Gets a reference to a value associated with a specific key.
     *
     * @warning This reference may be invalidated when the value is updated!
     *
     * @param key A key to be searched for.
     * @return The reference to the value associated with the specified key or
     *   to the default value specified during object construction if the key
     *   was not found.
     */
    const VALUE& get(const KEY& key)
    { // Finding requires only read access to the underlying map
      ScopedReadLock readlock(this->m_lock);

      // Check if the key is present in the underlying map
      typename Map::iterator it = m_map.find(key);

      if (it != m_map.end())
      { // Key found, return the associated value
        return it->second;
      }

      // Key not found, return the default value
      return m_defaultValue;
    }

    /**
     * Updates a specific key with a value associated with it. If the key is
     *   not present in the map, it will be inserted.
     *
     * @param key A key to be updated.
     * @param value A value associated with the key.
     */
    void update(const KEY& key, const VALUE& value)
    { // Inserting a key/value pair must be exclusive
      this->writelock();

      // Insert or update the key/value pair in the underlying map
      m_map[key] = value;

      // Key/value pair inserted, allow other threads to access the map
      this->unlock();
    }
};

#endif /* __PINTOOL_ANACONDA__UTILS__RWMAP_HPP__ */

/** End of file rwmap.hpp **/
