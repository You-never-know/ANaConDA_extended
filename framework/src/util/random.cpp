/**
 * @brief Contains implementation of functions for generating random numbers.
 *
 * A file containing implementation of functions for generating random numbers.
 *
 * @file      random.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-10
 * @date      Last Update 2013-05-14
 * @version   0.2
 */

#include "random.hpp"

namespace detail
{ // Implementation details, never use directly!

// Initialise internal global variables
RngEngine g_rng;
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

  // Initialise the random number generator
  g_rng.seed(static_cast< RngEngine::result_type >(settings->getSeed()));
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
