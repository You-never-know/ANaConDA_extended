/**
 * @brief Contains implementation of classes monitoring synchronisation
 *   coverage.
 *
 * A file containing implementation of classes monitoring synchronisation
 *   coverage.
 *
 * @file      sync.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-01-29
 * @date      Last Update 2013-02-06
 * @version   0.1
 */

#include <assert.h>

#include "sync.h"

namespace
{ // Static global variables (usable only within this module)
  /**
   * @brief An array holding a text description of the types of synchronisation
   *   coverage events (a text description of the @c EventType enumeration
   *   constants).
   */
  const char* g_eventTypeString[] = {
    "VISITED",
    "BLOCKED",
    "BLOCKING"
  };
}

/**
 * Acquires an object holding information about a synchronisation primitive.
 *
 * @tparam SP A type of the synchronisation primitive.
 * @tparam SI A type of the object holding information about the synchronisation
 *   primitive.
 *
 * @param sp A synchronisation primitive.
 * @param map A map containing objects holding information about each discovered
 *   synchronisation primitive.
 * @param lock A lock guarding the access to the map.
 * @return A reference to the object holding information about the specified
 *   synchronisation primitive.
 */
template < typename SP, typename SI >
inline
SI& acquire(SP& sp, std::map< SP, SI >& map, PIN_MUTEX& lock)
{
  // Other threads might be writing to the map, we need exclusive access
  PIN_MutexLock(&lock);

  // We need to get SI for the SP or create a new SI with default values
  SI& si = map[sp];

  // It is safe to allow the other threads to access the map as we already have
  // a reference to SI and it cannot be invalid as delete is never performed
  PIN_MutexUnlock(&lock);

  // We need exclusive access to SI, but other SIs may be accessed concurrently
  si.lock();

  // We successfully acquired the SI we wanted, other threads might access the
  // map and all other SIs as they wish, we only care about the SI we have now
  return si;
}

/**
 * Releases an object holding information about a synchronisation primitive so
 *   that other threads might access it.
 *
 * @tparam SI A type of the object holding information about the synchronisation
 *   primitive.
 *
 * @param si An object holding information about a synchronisation primitive.
 */
template < typename SI >
inline
void release(SI& si)
{
  si.unlock();
}

/**
 * Updates synchronisation coverage.
 *
 * @note This method is called when a thread wants to acquire a lock.
 *
 * @param l A lock.
 * @param ll A location at which is the thread trying to acquire the lock.
 */
void SynchronisationCoverage::beforeLockAcquired(LOCK l, index_t ll)
{
  // Get exclusive access to synchronisation information about the lock
  SyncInfo& si = acquire(l, m_lockMap, m_lockMapLock);

  // A thread is waiting for a lock (do not care which, it is irrelevant)
  si.waiting[ll]++;

  // Lock at the specified location was visited
  this->writeEvent(ll, ET_VISITED);

  if (si.holds)
  { // Some thread is holding the lock and is blocking other thread
    this->writeEvent(ll, ET_BLOCKED);
    this->writeEvent(si.holder, ET_BLOCKING);
  }

  // We are done, let the other threads access the sync info about the lock
  release(si);
}

/**
 * Updates synchronisation coverage.
 *
 * @note This method is called when a thread acquired a lock.
 *
 * @param l A lock.
 * @param ll A location at which the thread acquired the lock.
 */
void SynchronisationCoverage::afterLockAcquired(LOCK l, index_t ll)
{
  // Get exclusive access to synchronisation information about the lock
  SyncInfo& si = acquire(l, m_lockMap, m_lockMapLock);

  // A thread acquired a lock (and stopped waiting for it)
  si.holds = true;
  si.holder = ll;
  si.waiting[ll]--;

  for (IndexBag::iterator it = si.waiting.begin(); it != si.waiting.end(); it++)
  { // The thread might now be blocking other threads waiting for the same lock
    assert(it->second >= 0);

    if (it->second > 0)
    { // At least one thread started waiting for the lock at this location
      this->writeEvent(it->first, ET_BLOCKED);
      this->writeEvent(ll, ET_BLOCKING);
    }
  }

  // We are done, let the other threads access the sync info about the lock
  release(si);
}

/**
 * Updates synchronisation coverage.
 *
 * @note This method is called when a thread is about to release a lock.
 *
 * @param l A lock.
 * @param ll A location at which the thread is about to release the lock.
 */
void SynchronisationCoverage::beforeLockReleased(LOCK l, index_t ll)
{
  // Get exclusive access to synchronisation information about the lock
  SyncInfo& si = acquire(l, m_lockMap, m_lockMapLock);

  // A thread released a lock
  si.holds = false;

  // We are done, let the other threads access the sync info about the lock
  release(si);
}

/**
 * Writes a synchronisation coverage event to a file.
 *
 * @param l A location where the event occurred.
 * @param et A type of the event that occurred.
 */
void SynchronisationCoverage::writeEvent(index_t l, EventType et)
{
  // TODO: Write to file
  CONSOLE("SYNCCOV: " + retrieveCall(l) + " " + g_eventTypeString[et] + "\n");
}

/** End of file sync.cpp **/
