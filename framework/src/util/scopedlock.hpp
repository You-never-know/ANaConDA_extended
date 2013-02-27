/**
 * @brief Contains implementation of classes representing scoped locks.
 *
 * A file containing implementation of classes representing scoped locks.
 *
 * @file      scopedlock.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-27
 * @date      Last Update 2013-02-27
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__UTIL__SCOPEDLOCK_HPP__
  #define __PINTOOL_ANACONDA__UTIL__SCOPEDLOCK_HPP__

#include "pin.H"

/**
 * @brief A class representing a scoped lock.
 *
 * Represents a scoped lock. Objects of this class have a lock assigned to them.
 *   This lock is acquired when the object is created and released when it is
 *   destroyed.
 *
 * @tparam LT A type of locks assigned to objects of this class.
 * @tparam LOCKFUNPTR A function used to acquired the lock.
 * @tparam UNLOCKFUNPTR A function used to release the lock.
 */
template< typename LT, VOID (*LOCKFUNPTR)(LT*), VOID (*UNLOCKFUNPTR)(LT*) >
class ScopedLock
{
  private: // Internal variables
    /**
     * @brief A lock acquired during object construction and released when the
     *   object is destroyed.
     */
    LT& m_lock;
  public: // Constructors
    /**
     * Constructs a ScopedLock object and acquires a lock assigned to it.
     *
     * @param lock A lock which will be acquired when the object is constructed.
     */
    ScopedLock(LT& lock) : m_lock(lock) { LOCKFUNPTR(&m_lock); }
  public: // Destructors
    /**
     * Destroys a ScopedLock object and releases a lock assigned to it.
     */
    ~ScopedLock() { UNLOCKFUNPTR(&m_lock); }
};

typedef class ScopedLock< PIN_RWMUTEX, PIN_RWMutexReadLock, PIN_RWMutexUnlock >
  ScopedReadLock;
typedef class ScopedLock< PIN_RWMUTEX, PIN_RWMutexWriteLock, PIN_RWMutexUnlock >
  ScopedWriteLock;

#endif /* __PINTOOL_ANACONDA__UTIL__SCOPEDLOCK_HPP__ */

/** End of file scopedlock.hpp **/
