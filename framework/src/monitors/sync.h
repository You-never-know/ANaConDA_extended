/**
 * @brief Contains definitions of classes monitoring synchronisation coverage.
 *
 * A file containing definitions of classes monitoring synchronisation coverage.
 *
 * @file      sync.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-01-29
 * @date      Last Update 2013-05-30
 * @version   0.2.0.2
 */

#ifndef __PINTOOL_ANACONDA__COVERAGE__SYNC_H__
  #define __PINTOOL_ANACONDA__COVERAGE__SYNC_H__

#include <map>
#include <unordered_map>

#include "pin.H"

#include "../index.h"
#include "../types.h"

#include "../utils/lockobj.hpp"

// Type definitions
typedef std::unordered_map< index_t, int > IndexBag;

/**
 * @brief A class monitoring synchronisation coverage.
 *
 * Monitors synchronisation coverage.
 *
 * @tparam Writer A writer which should be used to write all the information
 *   about the synchronisation coverage.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-01-31
 * @date      Last Update 2013-02-07
 * @version   0.2
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
    void beforeLockAcquired(LOCK l, index_t ll);
    void afterLockAcquired(LOCK l, index_t ll);
    void beforeLockReleased(LOCK l, index_t ll);
  private: // Helper methods
    void writeEvent(index_t l, EventType et);
};

#endif /* __PINTOOL_ANACONDA__COVERAGE__SYNC_H__ */

/** End of file sync.h **/
