/*
 * Copyright (C) 2013-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

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
