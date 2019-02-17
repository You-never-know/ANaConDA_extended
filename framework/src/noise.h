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
 * @brief Contains definitions of classes for configuring noise injection.
 *
 * A file containing definitions of classes for managing noise generators, i.e.,
 *   functions generating noise, and holding the noise injection settings for
 *   various types of locations.
 *
 * @file      noise.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2015-07-21
 * @version   0.2.1
 */

#ifndef __PINTOOL_ANACONDA__NOISE_H__
  #define __PINTOOL_ANACONDA__NOISE_H__

#include <iostream>
#include <list>

#include "pin.H"

#include "utils/properties.hpp"

// Type definitions
typedef VOID (PIN_FAST_ANALYSIS_CALL *NOISEGENFUNPTR)(THREADID tid,
  UINT32 frequency, UINT32 strength);

/**
 * @brief An enumeration of filters which can be used to restrict the number of
 *   locations before which a noise might be injected.
 */
typedef enum NoiseFilter_e
{
  /**
   * @brief Shared variables filter.
   *
   * Noise might be placed before accesses to one or more shared variables.
   */
  NF_SHARED_VARS,
  /**
   * @brief Predecessors filter.
   *
   * Noise might be placed before accesses which have a predecessor (a previous
   *   access to the same variable in the same function).
   */
  NF_PREDECESSORS,
  /**
   * @brief Inverse noise filter.
   *
   * Noise might be placed before a location only if the inverse noise is not
   *   active.
   */
  NF_INVERSE_NOISE
} NoiseFilter;

// Type definitions
typedef std::list< NoiseFilter > NoiseFilterList;

/**
 * @brief A structure containing noise injection settings for a specific set of
 *   locations.
 */
typedef struct NoiseSettings_s
{
  /**
   * @brief A function used to determine if a noise might be injected before a
   *   specific location (instruction).
   */
  AFUNPTR filter;
  /**
   * @brief A list of filters which the @em filter function should use to
   *   determine if a noise might be injected before a specific location
   *   (instruction).
   */
  NoiseFilterList filters;
  /**
   * @brief A map containing properties (name/value pairs) of filters.
   *
   * Currently supported properties are:
   *   - @c svars.type @c = @em <std::string> where @em <std::string> might be:
   *     - @em all if the noise should be injected before all shared variables.
   *     - @em one if the noise should be injected before one randomly chosen
   *       variable.
   */
  Properties properties;
  NOISEGENFUNPTR generator; //!< A function generating noise.
  std::string gentype; //!< A type of a function generating noise.
  UINT32 frequency; //!< A probability that a noise will be inserted.
  UINT32 strength; //!< A strength of a noise.

  /**
   * Constructs a NoiseSettings_s object.
   */
  NoiseSettings_s() : filter(NULL), generator(NULL), gentype(), frequency(0),
    strength(0) {}

  /**
   * Constructs a NoiseSettings_s object.
   *
   * @param t A type of a function generating noise.
   * @param f A probability that the noise will be inserted.
   * @param s A strength of the noise.
   */
  NoiseSettings_s(std::string t, unsigned int f, unsigned int s) : filter(NULL),
      generator(NULL), gentype(t), frequency(f), strength(s) {}
} NoiseSettings;

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const NoiseSettings& value);

/**
 * @brief A class for registering and retrieving noise generators.
 *
 * Registers and provides functions generating noise.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2013-05-21
 * @version   0.2
 */
class NoiseGeneratorRegister
{
  public: // Type definitions
    typedef std::map< std::string, NOISEGENFUNPTR > NoiseGeneratorMap;
  private: // Static attributes
    static NoiseGeneratorRegister* ms_instance; //!< A singleton instance.
  private: // Internal variables
    /**
     * @brief A map containing functions generating noise.
     */
    NoiseGeneratorMap m_registeredNoiseGenerators;
  public: // Static methods
    static NoiseGeneratorRegister* Get();
  public: // Member methods for registering and retrieving noise generators
    NOISEGENFUNPTR getNoiseGenerator(std::string name);
    void registerNoiseGenerator(std::string name, NOISEGENFUNPTR generator);
};

// Macro definitions for simpler noise generator registration and retrieval
#define REGISTER_NOISE_GENERATOR(name, function) \
  NoiseGeneratorRegister::Get()->registerNoiseGenerator(name, function)
#define GET_NOISE_GENERATOR(name) \
  NoiseGeneratorRegister::Get()->getNoiseGenerator(name)

#endif /* __PINTOOL_ANACONDA__NOISE_H__ */

/** End of file noise.h **/
