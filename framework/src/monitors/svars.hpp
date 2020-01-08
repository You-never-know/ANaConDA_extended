/*
 * Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of a class monitoring shared variables.
 *
 * A file containing implementation of a class monitoring shared variables.
 *
 * @file      svars.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-09-23
 * @version   0.6
 */

#ifndef __PINTOOL_ANACONDA__MONITORS__SVARS_HPP__
  #define __PINTOOL_ANACONDA__MONITORS__SVARS_HPP__

#include <fstream>
#include <map>
#include <set>

#include <boost/foreach.hpp>

#include "pin.H"

#include "../types.h"

#include "../utils/scopedlock.hpp"

/**
 * @brief A class monitoring shared variables.
 *
 * Monitors shared variables.
 *
 * @tparam Writer A class used for writing the output.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-09-23
 * @version   0.6
 */
template< typename Writer >
class SharedVariablesMonitor : public Writer
{
  private: // Type definitions
    typedef std::map< std::string, std::set< THREADID > > VarMap;
  private: // Internal variables
    VarMap m_varMap; //!< A map containing information about variables.
    PIN_RWMUTEX m_varMapLock; //!< A lock guarding access to the variable map.
  public: // Constructors
    /**
     * Constructs a SharedVariablesMonitor object.
     */
    SharedVariablesMonitor() { PIN_RWMutexInit(&m_varMapLock); }
  public: // Destructors
    /**
     * Destroys a SharedVariablesMonitor object and writes the names of all
     *   shared variables detected to the output.
     */
    ~SharedVariablesMonitor()
    {
      PIN_RWMutexFini(&m_varMapLock);

      BOOST_FOREACH(const VarMap::value_type& item, m_varMap)
      { // Write names of variables accessed by more than one thread to output
        if (item.second.size() > 1) this->writeln(item.first);
      }
    }

  public: // Methods for loading shared variables
    /**
     * Loads shared variables from a file.
     *
     * @note This method does not check the existence of the file, if required,
     *   it should be done before calling this method.
     *
     * @param path A path to a file containing the shared variables.
     */
    void load(const std::string& path)
    {
      // Extract information about all shared variables from some previous run
      std::fstream f(path, std::fstream::in);

      // Helper variables
      std::string line;

      while (std::getline(f, line) && !f.fail())
      { // Each line contains the name of one shared variable
        if (line.empty()) continue;

        // Shared variable must be accessed by more than one thread
        m_varMap.insert(VarMap::value_type(line, {0, 1}));
      }
    }

  public: // Methods monitoring the shared variables
    /**
     * Updates a set of threads accessing a variable.
     *
     * @note This method is called before a thread accesses a variable.
     *
     * @param tid A thread accessing a variable.
     * @param addr An address on which is the variable stored.
     * @param var A variable accessed by a thread.
     * @param isLocal @em True if the variable is a local variable, @em false
     *   otherwise.
     */
    void beforeVariableAccessed(THREADID tid, ADDRINT addr, const VARIABLE& var,
      BOOL isLocal)
    {
      if (isLocal) return; // Local variable cannot be shared between threads

      // Other threads might be writing to the map, we need exclusive access
      ScopedWriteLock wrtlock(m_varMapLock);

      // For each variable, save the set of threads accessing this variable
      m_varMap[var.name.empty() ? hexstr(addr) : var.name].insert(tid);
    }

  public: // Methods for checking variables
    /**
     * Checks if a variable is a shared variable, i.e., is accessed by more than
     *   one thread.
     *
     * @param var A variable.
     * @return @em True if the variable is a shared variable, @em false
     *   otherwise.
     */
    bool isSharedVariable(const VARIABLE& var)
    {
      // Other threads might be reading from the map with us, no problem
      ScopedReadLock rdlock(m_varMapLock);

      VarMap::iterator it = m_varMap.find(var.name);

      // If more than one thread accessed the variable, it is a shared variable
      return (it != m_varMap.end() && it->second.size() > 1) ? true : false;
    }

  public: // Methods for accessing shared variables
    /**
     * Gets a list of shared variables detected so far.
     *
     * @return A list of shared variables detected so far.
     */
    std::vector< std::string > getSharedVariables()
    {
      // Helper variables
      std::vector< std::string > svars;

      // Other threads might be reading from the map with us, no problem
      ScopedReadLock rdlock(m_varMapLock);

      BOOST_FOREACH(const VarMap::value_type& item, m_varMap)
      { // Add all variables accessed by more than one thread to the list
        if (item.second.size() > 1) svars.push_back(item.first);
      }

      return svars; // Return all shared variables detected so far
    }
};

#endif /* __PINTOOL_ANACONDA__MONITORS__SVARS_HPP__ */

/** End of file svars.hpp **/
