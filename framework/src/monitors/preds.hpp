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
 * @brief Contains definitions of classes monitoring predecessors.
 *
 * A file containing definitions of classes monitoring predecessors.
 *
 * @file      preds.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-04-05
 * @date      Last Update 2013-09-24
 * @version   0.3
 */

#ifndef __PINTOOL_ANACONDA__MONITORS__PREDS_HPP__
  #define __PINTOOL_ANACONDA__MONITORS__PREDS_HPP__

#include <fstream>
#include <list>
#include <set>

#include <boost/foreach.hpp>

#include "pin.H"

#include "../types.h"

#include "../utils/scopedlock.hpp"
#include "../utils/tldata.hpp"

/**
 * @brief A class monitoring predecessors.
 *
 * Monitors predecessors.
 *
 * @tparam Writer A class used for writing the output.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-04-05
 * @date      Last Update 2013-09-24
 * @version   0.3
 */
template< typename Writer >
class PredecessorsMonitor : public Writer
{
  private: // Type definitions
    typedef std::set< ADDRINT > PredecessorSet;
    /**
     * @brief A structure holding private data of a thread.
     */
    typedef struct ThreadData_s
    {
      /**
       * @brief A set of variables accessed by a thread in a single function.
       */
      std::list< std::set< std::string > > vars;

      /**
       * Constructs a ThreadData_s object.
       */
      ThreadData_s() : vars()
      {
        vars.push_back(std::set< std::string >());
      }
    } ThreadData;
  private: // Internal variables
    PredecessorSet m_pSet; //!< A set containing instructions with predecessors.
    PIN_RWMUTEX m_pSetLock; //!< A lock guarding access to the predecessor set.
    ThreadLocalData< ThreadData > m_data; //!< Private data of running threads.
  public: // Constructors
    /**
     * Constructs a PredecessorsMonitor object.
     */
    PredecessorsMonitor() { PIN_RWMutexInit(&m_pSetLock); }
  public: // Destructors
    /**
     * Destroys a PredecessorsMonitor object and writes addresses of all
     *   instructions which have a predecessor to the output.
     */
    ~PredecessorsMonitor()
    {
      PIN_RWMutexFini(&m_pSetLock);

      BOOST_FOREACH(PredecessorSet::const_reference addr, m_pSet)
      { // Write addresses of instructions which have a predecessor to output
        this->writeln(hexstr(addr));
      }
    }

  public: // Methods for loading predecessors
    /**
     * Loads instructions with predecessors from a file.
     *
     * @note This method does not check the existence of the file, if required,
     *   it should be done before calling this method.
     *
     * @param path A path to a file containing instructions with predecessors.
     */
    void load(const std::string& path)
    {
      // Extract info about instructions with predecessors from a previous run
      std::fstream f(path, std::fstream::in);

      // Helper variables
      std::string line;

      while (std::getline(f, line) && !f.fail())
      { // Each line contains an address of one instruction with a predecessor
        if (line.empty()) continue;

        m_pSet.insert(AddrintFromString(line));
      }
    }

  public: // Methods monitoring the predecessors
    /**
     * Creates an empty set of variables accessed in a function.
     *
     * @note This method is called before a thread enters a function.
     *
     * @param tid A thread entering a function.
     */
    void beforeFunctionEntered(THREADID tid)
    {
      m_data.get(tid)->vars.push_back(std::set< std::string >());
    }

    /**
     * Deletes a set of variables accessed in a function.
     *
     * @note This method is called before a thread leaves a function.
     *
     * @param tid A thread leaving a function.
     */
    void beforeFunctionExited(THREADID tid)
    {
      m_data.get(tid)->vars.pop_back();
    }

    /**
     * Updates a set of instructions with predecessors.
     *
     * @note This method is called before a thread accesses a variable.
     *
     * @param tid A thread accessing a variable.
     * @param addr An address at which is the variable stored.
     * @param var A variable accessed by the thread.
     * @param ins An address of the instruction accessing the variable.
     * @param isLocal @em True if the variable is a local variable, @em false
     *   otherwise.
     */
    void beforeVariableAccessed(THREADID tid, ADDRINT addr, VARIABLE var,
      ADDRINT ins, BOOL isLocal)
    {
      if (isLocal) return; // Local variable cannot be shared between threads

      std::set< std::string >& vSet = m_data.get(tid)->vars.back();

      if (vSet.find(var.name.empty() ? hexstr(addr) : var.name) != vSet.end())
      { // This variable was accessed before
        ScopedWriteLock wrtlock(m_pSetLock);

        m_pSet.insert(ins); // The instruction has a predecessor
      }
      else
      { // This variable was not accessed before, remember it
        vSet.insert(var.name.empty() ? hexstr(addr) : var.name);
      }
    }

  public: // Methods for checking instructions
    /**
     * Checks if an instruction has a predecessor.
     *
     * @param ins An address of an instruction.
     * @return @em True if the instruction has a predecessor, @em false
     *   otherwise.
     */
    bool hasPredecessor(ADDRINT ins)
    {
      // Other threads might be reading from the map with us, no problem
      ScopedReadLock rdlock(m_pSetLock);

      // If the instruction is in the predecessor set, it has a predecessor
      return m_pSet.find(ins) != m_pSet.end();
    }
};

#endif /* __PINTOOL_ANACONDA__MONITORS__PREDS_HPP__ */

/** End of file preds.hpp **/
