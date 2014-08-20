/**
 * @brief Contains implementation of an interval map.
 *
 * A file containing implementation of an interval map.
 *
 * @file      ivalmap.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-07
 * @date      Last Update 2013-03-08
 * @version   0.3
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
 * @tparam KEY A type of data representing lower and upper bounds of intervals.
 * @tparam VALUE A type of data representing map's values.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-07
 * @date      Last Update 2013-03-08
 * @version   0.3
 */
template< typename KEY, typename VALUE >
class IntervalMap
{
  public: // Type definitions
    /**
     * @brief A structure holding the range (lower and upper bound) of the
     *   interval.
     */
    typedef struct Interval_s
    {
      KEY min; //!< A lower bound of the interval.
      KEY max; //!< An upper bound of the interval.

      /**
       * Constructs an Interval_s object.
       *
       * @param min A lower bound of the interval.
       * @param max An upper bound of the interval.
       */
      Interval_s(const KEY& min, const KEY& max) : min(min), max(max) {}

      /**
       * Checks if the lower bound of this interval is lesser than the lower
       *   bound of some other interval.
       *
       * @param ival An interval to compare with this interval.
       * @return @em True if the lower bound of this interval is lesser than
       *   the lower bound of the @em ival interval.
       */
      bool operator< (const Interval_s& ival) const
      {
        return min < ival.min;
      }

      /**
       * Checks if the lower bound of this interval is greater than the lower
       *   bound of some other interval.
       *
       * @param ival An interval to compare with this interval.
       * @return @em True if the lower bound of this interval is greater than
       *   the lower bound of the @em ival interval.
       */
      bool operator> (const Interval_s& ival) const
      {
        return min > ival.min;
      }
    } Interval;
    typedef std::map< Interval, VALUE, std::greater< Interval > > Map;
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
      iterator it = m_map.lower_bound(Interval(key, key));

      // Lower bound will give us the only interval which may contain the key
      if (it->first.min <= key && key < it->first.max) return it;

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
      return m_map.insert(typename Map::value_type(Interval(min, max), value));
    }
};

#endif /* __LIBPIN_DIE__UTIL__IVALMAP_HPP__ */

/** End of file ivalmap.hpp **/
