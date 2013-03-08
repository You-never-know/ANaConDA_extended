/**
 * @brief Contains implementation of an interval map.
 *
 * A file containing implementation of an interval map.
 *
 * @file      ivalmap.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-07
 * @date      Last Update 2013-03-08
 * @version   0.2
 */

#ifndef __LIBPIN_DIE__UTIL__IVALMAP_HPP__
  #define __LIBPIN_DIE__UTIL__IVALMAP_HPP__

#include <map>

/**
 * @brief A class representing an interval map.
 *
 * Represents a map where keys are intervals (of unsigned numbers).
 *
 * @note The implementation assumes [@em min, @em max) intervals, so the @em min
 *   value is considered to be in the interval, but the @em max value not.
 *
 * @warning The implementation is not thread-safe!
 *
 * @tparam KEY A type of data representing map's keys.
 * @tparam VALUE A type of data representing map's values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-07
 * @date      Last Update 2013-03-08
 * @version   0.2
 */
template< typename KEY, typename VALUE >
class IntervalMap
{
  public: // Type definitions
    /**
     * @brief A structure holding the value of the element together with the
     *   upper bound of the interval.
     */
    typedef struct Interval_s
    {
      KEY max; //!< An upper bound of the interval.
      VALUE value; //!< A value associated with the interval.

      Interval_s(const KEY& m, const VALUE& v) : max(m), value(v) {}
    } Interval;
    typedef std::map< KEY, Interval, std::greater< KEY > > Map;
    typedef typename Map::iterator iterator;
  private: // Internal variables
    Map m_map; //!< An underlying map containing key/value pairs.
  public: // Inline generated methods
    /**
     * Finds an element associated with an interval to which a key belongs.
     *
     * @param key A key to be searched for.
     * @return An iterator to the element associated with the interval to which
     *   the key belongs or IntervalMap::end() if the key does not belong to any
     *   interval in the map.
     */
    iterator find(const KEY& key)
    {
      // Try to find the interval to which the key belongs
      iterator it = m_map.lower_bound(key);

      // Lower bound will give us the only interval which may contain the key
      if (it->first <= key && key < it->second.max) return it;

      // No interval found
      return m_map.end();
    }

    /**
     * Returns an iterator referring to the @em past-the-end element in the map
     *   container.
     *
     * @return An iterator to the @em past-the-end element in the container.
     */
    iterator end()
    {
      return m_map.end();
    }

    /**
     * Inserts an interval with a value associated with it.
     *
     * @param min A lower bound of the interval.
     * @param max An upper bound of the interval.
     * @param value A value associated with the interval.
     */
    std::pair< iterator, bool >
    insert(const KEY& min, const KEY& max, const VALUE& value)
    {
      return m_map.insert(typename Map::value_type(min, Interval(max, value)));
    }
};

#endif /* __LIBPIN_DIE__UTIL__IVALMAP_HPP__ */

/** End of file ivalmap.hpp **/
