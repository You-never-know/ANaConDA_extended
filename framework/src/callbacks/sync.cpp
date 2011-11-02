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
 * @date      Last Update 2011-11-02
 * @version   0.1.1
 */

#include "sync.h"

#include <iostream>

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock A location representing the lock.
 */
VOID beforeLockAcquire(THREADID tid, ADDRINT lock)
{
  std::cout << "Acquired lock " << lock << std::endl;
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock A location representing the lock.
 */
VOID beforeLockRelease(THREADID tid, ADDRINT lock)
{
  std::cout << "Released lock " << lock << std::endl;
}

/** End of file sync.cpp **/
