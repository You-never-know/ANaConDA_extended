/**
 * @brief Contains implementation of a lockable object.
 *
 * A file containing implementation of a class providing thread-safe access to
 *   its members. The access is guarded by a lock.
 *
 * @file      lockobj.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-06
 * @date      Last Update 2016-02-28
 * @version   0.2
 */

#ifndef __PINTOOL_ANACONDA__UTILS__LOCKOBJ_HPP__
  #define __PINTOOL_ANACONDA__UTILS__LOCKOBJ_HPP__

#include "pin.H"

/**
 * @brief A class representing a lockable object.
 *
 * Provides thread-safe access to class members. The access is guarded by a lock.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-06
 * @date      Last Update 2013-02-06
 * @version   0.1
 */
class LockableObject
{
  private:
    PIN_MUTEX m_lock; //!< A lock guarding access to class members.
  public:
    /**
     * Constructs a LockableObject object.
     */
    LockableObject() { PIN_MutexInit(&m_lock); }
    /**
     * Destroys a LockableObject object.
     */
    ~LockableObject() { PIN_MutexFini(&m_lock); }
  public:
    /**
     * Acquires a lock guarding access to class members.
     */
    void lock() { PIN_MutexLock(&m_lock); }
    /**
     * Releases a lock guarding access to class members.
     */
    void unlock() { PIN_MutexUnlock(&m_lock); }
};

/**
 * @brief A class representing a read/write lockable object.
 *
 * Provides thread-safe read/write access to class members. The read/write
 *   access is guarded by a lock. More that one thread may read at a time,
 *   however, only one thread may write at a time and during this time, no
 *   reading is allowed in the other threads.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-28
 * @date      Last Update 2016-02-28
 * @version   0.1
 */
class RwLockableObject
{
  private:
    PIN_RWMUTEX m_lock; //!< A lock guarding read/write access to class members.
  public:
    /**
     * Constructs a RwLockableObject object.
     */
    RwLockableObject() { PIN_RWMutexInit(&m_lock); }
    /**
     * Destroys a RwLockableObject object.
     */
    ~RwLockableObject() { PIN_RWMutexFini(&m_lock); }
  public:
    /**
     * Acquires a lock guarding read access to class members.
     */
    void readlock() { PIN_RWMutexReadLock(&m_lock); }
    /**
     * Acquires a lock guarding write access to class members.
     */
    void writelock() { PIN_RWMutexWriteLock(&m_lock); }
    /**
     * Releases a lock guarding access to class members.
     */
    void unlock() { PIN_RWMutexUnlock(&m_lock); }
};

#endif /* __PINTOOL_ANACONDA__UTILS__LOCKOBJ_HPP__ */

/** End of file lockobj.hpp **/
