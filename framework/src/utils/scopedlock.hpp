/*
 * Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Contains implementation of classes representing scoped locks.
 *
 * A file containing implementation of classes representing scoped locks.
 *
 * @file      scopedlock.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-27
 * @date      Last Update 2013-05-30
 * @version   0.2.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__SCOPEDLOCK_HPP__
  #define __PINTOOL_ANACONDA__UTILS__SCOPEDLOCK_HPP__

#include "pin.H"

/**
 * @brief A class implementing a generic scoped lock.
 *
 * Implements a generic scoped lock. Objects of this class have a lock assigned
 *   to them. This lock is acquired when the object is created and released when
 *   it is destroyed.
 *
 * @tparam LT A type of locks assigned to objects of this class.
 * @tparam LOCKFUNPTR A function used to acquired the lock.
 * @tparam UNLOCKFUNPTR A function used to release the lock.
 */
template< typename LT, VOID (*LOCKFUNPTR)(LT*), VOID (*UNLOCKFUNPTR)(LT*) >
class ScopedLockImpl
{
  private: // Internal variables
    /**
     * @brief A lock acquired during object construction and released when the
     *   object is destroyed.
     */
    LT& m_lock;
  public: // Constructors
    /**
     * Constructs a ScopedLockImpl object and acquires a lock assigned to it.
     *
     * @param lock A lock which will be acquired when the object is constructed.
     */
    ScopedLockImpl(LT& lock) : m_lock(lock) { LOCKFUNPTR(&m_lock); }
  public: // Destructors
    /**
     * Destroys a ScopedLockImpl object and releases a lock assigned to it.
     */
    ~ScopedLockImpl() { UNLOCKFUNPTR(&m_lock); }
};

typedef class ScopedLockImpl< PIN_RWMUTEX, PIN_RWMutexReadLock, PIN_RWMutexUnlock >
  ScopedReadLock;
typedef class ScopedLockImpl< PIN_RWMUTEX, PIN_RWMutexWriteLock, PIN_RWMutexUnlock >
  ScopedWriteLock;
typedef class ScopedLockImpl < PIN_MUTEX, PIN_MutexLock, PIN_MutexUnlock >
  ScopedLock;

#endif /* __PINTOOL_ANACONDA__UTILS__SCOPEDLOCK_HPP__ */

/** End of file scopedlock.hpp **/
