/**
 * @brief A file containing definitions of synchronisation-related callback
 *   functions.
 *
 * A file containing definitions of callback functions called when some
 *   synchronisation between threads occurs.
 *
 * @file      sync.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2011-11-07
 * @version   0.1.2
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__

#include "pin.H"

// Definitions of classes representing synchronisation primitives
typedef class INDEX< 200 > LOCK; //!< A class representing a lock.

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const LOCK& value);

VOID beforeLockAcquire(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc);
VOID beforeLockRelease(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__ */

/** End of file sync.h **/
