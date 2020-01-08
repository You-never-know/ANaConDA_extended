/*
 * Copyright (C) 2016-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-24
 * @version   0.5.1
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <string>
#include <vector>

#include "fa.hpp"

/**
 * @brief A structure representing a target.
 */
typedef struct Target_s
{
  typedef size_t Type; //!< A representation of the type of the target.

  Type type; //!< A type of the target.
  FA* fa; //!< A finite automaton representing the target.
  /**
   * @brief A set of spoilers which may violate the target.
   */
  std::vector< struct Spoiler_s* > spoilers;

  /**
   * Constructs a new target.
   *
   * @param t A type of the target.
   */
  Target_s(Type t) : type(t), fa(NULL) {};
} Target;

/**
 * @brief A structure representing a spoiler.
 */
typedef struct Spoiler_s
{
  typedef size_t Type; //!< A representation of the type of the spoiler.

  Type type; //!< A type of the spoiler.
  FA* fa; //!< A finite automaton representing the spoiler.
  struct Target_s* target; //!< A target that may be violated by the spoiler.

  /**
   * Constructs a new spoiler.
   *
   * @param t A type of the spoiler.
   */
  Spoiler_s(Type t) : type(t), fa(NULL), target(NULL) {};
} Spoiler;

/**
 * @brief A class representing a contract.
 *
 * Represents a contract.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-24
 * @version   0.4.1
 */
class Contract
{
  private: // Internal data
    /**
     * @brief A set of targets included in the contract.
     */
    std::vector< Target* > m_targets;
  public: // Destructors
    ~Contract();
  public: // Methods for loading a contract
    void load(const std::string& path);
  public: // Methods for accessing internal data
    const std::vector< Target* >& getTargets() { return m_targets; }
  public: // Methods for serialising a contract
    std::string toString();
  private: // Methods for transforming regular expressions to FAs
    FA* construct(const std::string& regex);
  private: // Methods for refining FAs
    FA* toEpsilonFreeFA(FA* fa);
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
