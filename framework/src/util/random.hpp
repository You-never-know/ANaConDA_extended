/**
 * @brief Contains definitions and implementation of functions for generating
 *   random numbers.
 *
 * A file containing definitions and implementation of functions for generating
 *   random numbers.
 *
 * @file      random.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-10
 * @date      Last Update 2013-05-13
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__UTIL__RANDOM_HPP__
  #define __PINTOOL_ANACONDA__UTIL__RANDOM_HPP__

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../settings.h"

#include "scopedlock.hpp"

namespace detail
{ // Implementation details, never use directly!

extern boost::random::mt11213b g_rng; //!< A random number generator.
/**
 * @brief A lock guarding access to the random number generator.
 *
 * Only the thread holding this lock might use the random number generator.
 */
extern PIN_MUTEX g_rngLock;

/**
 * Generates a random integer from the <@em min , @em max> interval.
 *
 * @tparam IT A type of the integer generated.
 *
 * @param min A minimal integer which might be generated.
 * @param max A maximal integer which might be generated.
 * @return An integer from the <@em min , @em max> interval.
 */
template< typename IT >
inline
IT randomInt(IT min, IT max)
{
  // Restrict the generated integer to the <min, max> interval
  boost::random::uniform_int_distribution<> dist(min, max);

  // Random number generation is not thread-safe, must be done exclusively
  ScopedLock lock(detail::g_rngLock);

  // Generate a new random integer from the <min, max> interval
  return dist(detail::g_rng);
}

} // namespace detail

/**
 * Generates a random integer from the <@em min , @em max> interval.
 *
 * @tparam IT A type of the integer generated. Default is @c UINT32.
 *
 * @param min A minimal integer which might be generated.
 * @param max A maximal integer which might be generated.
 * @return An integer from the <@em min , @em max> interval.
 */
template< typename IT = UINT32 >
inline
IT randomInt(IT min, IT max)
{
  return detail::randomInt< IT, IT >(min, max); // Concrete implementation
}

// Definitions of helper functions
VOID setupRandomModule(Settings* settings);

#endif /* __PINTOOL_ANACONDA__UTIL__RANDOM_HPP__ */

/** End of file random.hpp **/
