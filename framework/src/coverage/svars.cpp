/**
 * @brief Contains implementation of classes monitoring shared variables.
 *
 * A file containing implementation of classes monitoring shared variables.
 *
 * @file      svars.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-02-27
 * @version   0.1
 */

#include "svars.h"

#include "../util/scopedlock.hpp"
#include "../util/writers.h"

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

// Instantiate the monitor for various writers
template class SharedVarsMonitor< FileWriter >;

/** End of file svars.cpp **/
