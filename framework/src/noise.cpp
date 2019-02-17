/*
 * Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of classes for configuring noise injection.
 *
 * A file containing implementation of classes for managing noise generators,
 *   i.e., functions generating noise, and holding the noise injection settings
 *   for various types of locations.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2013-05-21
 * @version   0.2
 */

#include "noise.h"

/**
 * Prints a noise injection settings to a stream.
 *
 * @param s A stream to which the noise injection settings should be printed.
 * @param value A structure containing the noise injection settings.
 * @return The stream to which were the noise injection settings printed.
 */
std::ostream& operator<<(std::ostream& s, const NoiseSettings& value)
{
  // The parameters of the noise are always the same, only type may differ
  s << value.gentype << "(" << value.frequency << "," << value.strength << ")";

  return s; // Return the stream to which was the noise description printed
}

// Initialisation of the singleton instance
NoiseGeneratorRegister* NoiseGeneratorRegister::ms_instance = NULL;

/**
 * Gets a singleton instance.
 *
 * @note If no singleton instance exist, the method will create one.
 *
 * @return The singleton instance.
 */
NoiseGeneratorRegister* NoiseGeneratorRegister::Get()
{
  if (ms_instance == NULL)
  { // No singleton instance yet, create one
    ms_instance = new NoiseGeneratorRegister();
  }

  return ms_instance;
}

/**
 * Gets a noise generator, i.e., a function generating noise.
 *
 * @param name A name identifying the noise generator.
 * @return A noise generator, i.e., a function generating noise, or @em NULL if
 *   no noise generator with the specified name is registered.
 */
NOISEGENFUNPTR NoiseGeneratorRegister::getNoiseGenerator(std::string name)
{
  // Try to find the noise generator with the specified name
  NoiseGeneratorMap::iterator it = m_registeredNoiseGenerators.find(name);

  // Return the noise generator if it was found
  return (it != m_registeredNoiseGenerators.end()) ? it->second : NULL;
}

/**
 * Registers a noise generator, i.e., a function generating noise.
 *
 * @warning If a noise generator with the specified name already exist, it will
 *   be overwritten by the newly registered noise generator.
 *
 * @param name A name identifying the noise generator.
 * @param function A noise generator, i.e., a function generating noise.
 */
void NoiseGeneratorRegister::registerNoiseGenerator(std::string name,
  NOISEGENFUNPTR generator)
{
  m_registeredNoiseGenerators[name] = generator;
}

/** End of file noise.cpp **/
