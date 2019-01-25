/*
 * Copyright (C) 2016-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @date      Last Update 2019-01-25
 * @version   0.5.1.1
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <string>
#include <vector>
#include <stdexcept>
#include <utility> // pair
#include <cctype>

#include "pin.H"

#include "utils/lockobj.hpp"

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
class Contract : public RWLockableObject
{
  public:
    typedef std::vector<std::pair<std::string, int>> ParamFunList;
  private: // Internal data
    std::string original_line; // original str definition
    std::string argument; // argument of parameterized contract

    /**
     * @brief List of names of functions with positional argument to be
     * parametrized. Set by parsing the settings of a contract and used by
     * registering PIN event.
     */
    ParamFunList parametrized;

    /**
     * @brief A set of targets included in the contract.
     */
    std::vector< Target* > m_targets;
  public: // Ctor + Dtor
    Contract() {}
    Contract(const Contract& c, const std::string& arg);
    ~Contract();
  public: // Methods for loading a contract
    void load(const std::string& path);
    void parse(const std::string& line, const std::string& arg);
    const std::string& get_argument() { return argument; }
  public: // Methods for accessing internal data
    const std::vector< Target* >& getTargets() { return m_targets; }
    const ParamFunList &get_parametrized() { return parametrized; }
  public: // Methods for serialising a contract
    std::string toString();
  private: // Methods for transforming regular expressions to FAs
    /// Add name of parametrized function to the list of to-be-registered
    void add_parametrized(std::string fname)
    {
      size_t dollarpos = fname.find('$');
      if (dollarpos == std::string::npos)
        return;
      if (dollarpos + 1 >= fname.size() ||
          !std::isdigit(fname[dollarpos+1]))
        throw std::invalid_argument(
            "contract specification: '$' must follow with a single digit");
      int pos = fname[dollarpos+1] - '0';
      fname.erase(dollarpos);
      auto param = std::make_pair(fname, pos);
      for (auto &p : parametrized)
        if (p == param)
          return;
      parametrized.push_back(param);
    }
    FA* construct(const std::string& regex);
  private: // Methods for refining FAs
    static FA* toEpsilonFreeFA(FA* fa);
};

typedef std::vector<Contract*> ContractList;

/**
 * @brief A class representing list of contracts. The list is required when
 * registering a new, parametrized, contract. Registered, non-parameterized
 * contracts are used as templates for parametrized contracts. Most probably
 * used as a singleton in a single validator run.
 */
class Contracts
{
public:
  PIN_MUTEX g_Lock; //!< A lock guarding access to @c g_contracts list.

  ContractList g_contracts; //!< A list of contracts to be checked.

  /**
   * Register a new contract based on an argument. A new parametrized contract
   * is created for each yet-not-parameterized contract. All the contracts are
   * stored in `g_contracts' attribute. Thread-unsafe.
   * @param arg argument
   * @return list of new contracts
   * @pre All the contracts refer to a single argument. An argument is
   * valid for each parametrized function mentioned in contract.
   */
  ContractList add_parameter(const std::string &arg);

public:
  /**
   * A class locking mechanism for thread safety.
   */
  void lock() { PIN_MutexLock(&g_Lock); }
  void unlock() { PIN_MutexUnlock(&g_Lock); }
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
