/**
 * @brief Contains definitions of classes monitoring synchronisation coverage.
 *
 * A file containing definitions of classes monitoring synchronisation coverage.
 *
 * @file      sync.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-01-29
 * @date      Last Update 2013-06-05
 * @version   0.3
 */

#ifndef __PINTOOL_ANACONDA__MONITORS__SYNC_HPP__
  #define __PINTOOL_ANACONDA__MONITORS__SYNC_HPP__

#include <assert.h>

#include <map>
#include <unordered_map>

#include <boost/foreach.hpp>

#include "pin.H"

#include "../index.h"
#include "../types.h"

#include "../utils/lockobj.hpp"

// Type definitions
typedef std::unordered_map< index_t, int > IndexBag;

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
 * @brief A class monitoring synchronisation coverage.
 *
 * Monitors synchronisation coverage.
 *
 * @tparam Writer A class used for writing the output.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-01-31
 * @date      Last Update 2013-06-05
 * @version   0.3
 */
template< typename Writer >
class SyncCoverageMonitor : public Writer
{
  /**
   * @brief A structure holding information about a synchronisation primitive.
   */
  typedef struct SyncInfo_s : public LockableObject
  {
    bool holds; //!< A flag indicating if some thread is holding the primitive.
    index_t holder; //!< An index of the location holding the primitive.
    IndexBag waiting; //!< A list of location indices waiting for the primitive.

    /**
     * Constructs a SyncInfo_s object.
     */
    SyncInfo_s() : LockableObject(), holds(false), holder(), waiting() {}
  } SyncInfo;

  /**
   * @brief An enumeration of types of events which might occur when monitoring
   *   synchronisation coverage.
   */
  typedef enum EventType_e
  {
    /**
     * @brief Thread reached a synchronisation function.
     */
    ET_VISITED,
    /**
     * @brief Thread T_1 was blocked by thread T_2 while attempting to acquire
     *   a lock which is currently acquired by thread T_2.
     */
    ET_BLOCKED,
    /**
     * @brief Thread T_1 is blocking thread T_2 because thread T_2 attempted to
     *   acquire the same lock which is currently acquired by thread T_2.
     */
    ET_BLOCKING
  } EventType;

  private: // Type definitions
    typedef std::map< LOCK, SyncInfo > LockInfoMap;
  private: // Internal variables
    LockInfoMap m_lockMap; //!< A map containing information about locks.
    PIN_MUTEX m_lockMapLock; //!< A lock guarding access to the lock map.
  public: // Constructors
    /**
     * Constructs a SynchronisationCoverage object.
     */
    SyncCoverageMonitor() { PIN_MutexInit(&m_lockMapLock); }
  public: // Destructors
    /**
     * Destroys a SynchronisationCoverage object.
     */
    ~SyncCoverageMonitor() { PIN_MutexFini(&m_lockMapLock); }
  public: // Methods monitoring the synchronisation coverage
    /**
     * Updates synchronisation coverage.
     *
     * @note This method is called when a thread wants to acquire a lock.
     *
     * @param l A lock.
     * @param ll A location at which is the thread trying to acquire the lock.
     */
    void beforeLockAcquired(LOCK l, index_t ll)
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
    void afterLockAcquired(LOCK l, index_t ll)
    {
      // Get exclusive access to synchronisation information about the lock
      SyncInfo& si = acquire(l, m_lockMap, m_lockMapLock);

      // A thread acquired a lock (and stopped waiting for it)
      si.holds = true;
      si.holder = ll;
      si.waiting[ll]--;

      BOOST_FOREACH(IndexBag::const_reference item, si.waiting)
      { // The thread might be blocking other threads waiting for the same lock
        assert(item.second >= 0);

        if (item.second > 0)
        { // At least one thread started waiting for the lock at this location
          this->writeEvent(item.first, ET_BLOCKED);
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
    void beforeLockReleased(LOCK l, index_t ll)
    {
      // Get exclusive access to synchronisation information about the lock
      SyncInfo& si = acquire(l, m_lockMap, m_lockMapLock);

      // A thread released a lock
      si.holds = false;

      // We are done, let the other threads access the sync info about the lock
      release(si);
    }

  private: // Helper methods
    /**
     * Writes a synchronisation coverage event to the output.
     *
     * @param l A location where the event occurred.
     * @param et A type of the event that occurred.
     */
    void writeEvent(index_t l, EventType et)
    {
      // Text representation of the events, using static constexpr should force
      // the compiler to use only a single table for the whole class and put the
      // table into a read-only memory (where the code is) as the content never
      // changes (the values are constant and known in compile-time)
      static constexpr const char* eventTypeString[] = {
        "VISITED",
        "BLOCKED",
        "BLOCKING"
      };

      // Format (each line): <location> <event-type>
      this->writeln(retrieveCall(l) + " " + eventTypeString[et]);
    }
};

#endif /* __PINTOOL_ANACONDA__MONITORS__SYNC_HPP__ */

/** End of file sync.hpp **/
