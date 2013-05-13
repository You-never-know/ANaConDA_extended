/**
 * @brief Contains implementation of functions for generating random numbers.
 *
 * A file containing implementation of functions for generating random numbers.
 *
 * @file      random.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-10
 * @date      Last Update 2013-05-13
 * @version   0.1
 */

#include "random.hpp"

namespace detail
{ // Implementation details, never use directly!

// Initialise internal global variables
boost::random::mt11213b g_rng;
PIN_MUTEX g_rngLock;

/**
 * Setups the random number generation module. Initialises the random number
 *   generator and locks ensuring exclusive access to it.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
inline
VOID setupRandomModule(Settings* settings)
{
  // Initialise a lock guarding access to the random number generator
  PIN_MutexInit(&g_rngLock);

  // This method should never be called concurrently, so no need to guard g_rng
  g_rng.seed(static_cast< unsigned int >(std::time(0)));
}

} // namespace detail

/**
 * Setups the random number generation module. Initialises the random number
 *   generator and locks ensuring exclusive access to it.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupRandomModule(Settings* settings)
{
  detail::setupRandomModule(settings); // Concrete implementation
}

/** End of file random.cpp **/
