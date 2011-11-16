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
 * @date      Last Update 2011-11-16
 * @version   0.1.4
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__

#include "pin.H"

// Definitions of classes representing synchronisation primitives
typedef class INDEX< 200 > LOCK; //!< A class representing a lock.
typedef class INDEX< 201 > COND; //!< A class representing a condition.

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const LOCK& value);
std::ostream& operator<<(std::ostream& s, const COND& value);

VOID beforeLockAcquire(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc);
VOID beforeLockRelease(THREADID tid, ADDRINT* lockAddr, VOID* funcDesc);
VOID beforeSignal(THREADID tid, ADDRINT* condAddr, VOID* funcDesc);
VOID beforeWait(THREADID tid, ADDRINT* condAddr, VOID* funcDesc);

VOID afterLockAcquire(THREADID tid);
VOID afterLockRelease(THREADID tid);
VOID afterSignal(THREADID tid);
VOID afterWait(THREADID tid);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__ */

/** End of file sync.h **/
