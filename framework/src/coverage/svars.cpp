/**
 * @brief Contains implementation of classes monitoring shared variables.
 *
 * A file containing implementation of classes monitoring shared variables.
 *
 * @file      svars.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-04-23
 * @version   0.4
 */

#include "svars.h"

#include "../util/scopedlock.hpp"
#include "../util/writers.h"

#include <fstream>

/**
 * Destroys a SharedVarsMonitor object and writes all shared variables detected
 *   to a file.
 */
template< typename Writer >
SharedVarsMonitor< Writer >::~SharedVarsMonitor()
{
  PIN_RWMutexFini(&m_varMapLock);

  for (VarMap::iterator it = m_varMap.begin(); it != m_varMap.end(); it++)
  { // Write all variables accessed by more than one thread to output file
    if (it->second.size() > 1) this->writeln(it->first);
  }
}

/**
 * Loads shared variables from a file.
 *
 * @note This method does not check the existence of the file, if required, it
 *   should be done before calling this method.
 *
 * @param path A path to a file containing shared variables.
 */
template< typename Writer >
void SharedVarsMonitor< Writer >::load(const std::string& path)
{
  // Extract information about all shared variables from some previous run
  std::fstream f(path, std::fstream::in);

  // Helper variables
  std::string line;

  while (std::getline(f, line) && !f.fail())
  { // Each line containing the name of one shared variable
    if (line.empty()) continue;

    // Shared variable must be accessed by more than one thread
    m_varMap.insert(VarMap::value_type(line, {0, 1}));
  }
}

/**
 * Updates a set of threads accessing a variable.
 *
 * @note This method is called before a thread accesses a variable.
 *
 * @param tid A thread accessing a variable.
 * @param var A variable accessed by a thread.
 */
template< typename Writer >
void SharedVarsMonitor< Writer >::beforeVariableAccessed(THREADID tid,
  VARIABLE var)
{
  // Other threads might be writing to the map, we need exclusive access
  ScopedWriteLock wrtlock(m_varMapLock);

  // For each variable, save the set of threads accessing this variable
  m_varMap[var.name].insert(tid);
}

/**
 * Checks if a variable is a shared variable (accessed by more that one thread).
 *
 * @param var A variable.
 * @return @em True if the variable is a shared variable, @em false otherwise.
 */
template< typename Writer >
bool SharedVarsMonitor< Writer >::isSharedVariable(VARIABLE var)
{
  // Other threads might be reading from the map with us, no problem
  ScopedReadLock rdlock(m_varMapLock);

  VarMap::iterator it = m_varMap.find(var.name);

  // If more than one thread accessed the variable, it is a shared variable
  return (it != m_varMap.end() && it->second.size() > 1) ? true : false;
}

/**
 * Gets a list of shared variables detected so far.
 *
 * @return A list of shared variables detected so far.
 */
template< typename Writer >
std::vector< std::string > SharedVarsMonitor< Writer >::getSharedVariables()
{
  // Helper variables
  std::vector< std::string > svars;

  // Other threads might be reading from the map with us, no problem
  ScopedReadLock rdlock(m_varMapLock);

  for (VarMap::iterator it = m_varMap.begin(); it != m_varMap.end(); it++)
  { // Add all variables accessed by more than one thread to the list
    if (it->second.size() > 1) svars.push_back(it->first);
  }

  return svars; // Return all shared variables detected so far
}

// Instantiate the monitor for various writers
template class SharedVarsMonitor< FileWriter >;

/** End of file svars.cpp **/
