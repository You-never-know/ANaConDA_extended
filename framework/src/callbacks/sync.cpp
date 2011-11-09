/**
 * @brief A file containing implementation of synchronisation-related callback
 *   functions.
 *
 * A file containing implementation of callback functions called when some
 *   synchronisation between threads occurs.
 *
 * @file      sync.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2011-11-09
 * @version   0.1.2.1
 */

#include "sync.h"

#include <iostream>

#include "../settings.h"

/**
 * Gets a lock object representing a lock at a specific address.
 *
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock at the specified address.
 * @return The lock object representing the lock at the specified address.
 */
inline
LOCK getLock(ADDRINT* lockAddr, FunctionDesc* funcDesc)
{
  for (int lvl = funcDesc->plvl; lvl > 0; lvl--)
  { // If the pointer do not point to the address of the lock, get to it
    lockAddr = reinterpret_cast< ADDRINT* >(*lockAddr);
  }

  // Lock objects must be created in two steps, first create a lock object
  LOCK lock;
  // Then modify it to create a lock object for the specified address
  lock.q_set(funcDesc->farg->map(lockAddr));

  // Return the lock object representing a lock at the specified address
  return lock;
}

/**
 * Prints a lock object to a stream.
 *
 * @param s A stream to which the lock object should be printed.
 * @param value A lock object.
 * @return The stream to which was the lock object printed.
 */
std::ostream& operator<<(std::ostream& s, const LOCK& value)
{
  // Print the lock object to the stream
  s << "LOCK(index=" << value.q() << ")";

  // Return the stream to which was the lock object printed
  return s;
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockAcquire(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc)
{
  std::cout << "Acquired lock " << getLock(lockAddr,
    static_cast< FunctionDesc* >(funcDesc)) << std::endl << std::flush;
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lockAddr An address at which is the lock stored.
 * @param funcDesc A structure containing the description of the function
 *   working with the lock.
 */
VOID beforeLockRelease(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc)
{
  std::cout << "Released lock " << getLock(lockAddr,
    static_cast< FunctionDesc* >(funcDesc)) << std::endl << std::flush;
}

/** End of file sync.cpp **/
