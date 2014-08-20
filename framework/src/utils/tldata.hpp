/**
 * @brief Contains a class simplifying management of thread local data.
 *
 * A file containing a class simplifying management of thread local data.
 *
 * @file      tldata.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-31
 * @date      Last Update 2013-06-04
 * @version   0.2.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__TLDATA_HPP__
  #define __PINTOOL_ANACONDA__UTILS__TLDATA_HPP__

#include "pin.H"

#include "thread.h"

/**
 * @brief Simplifies management of thread local data.
 *
 * Encapsulates arbitrary data and provides access to them. Ensures automatic
 *   initialisation and proper release of this data.
 *
 * @warning A thread may access local data of another thread. Note, however,
 *   that the user must ensure proper synchronisation in this case to prevent
 *   concurrency errors like data races.
 *
 * @tparam T A type of data the class should manage.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-31
 * @date      Last Update 2013-06-03
 * @version   0.2
 */
template< typename T >
class ThreadLocalData
{
  private: // Internal variables
    /**
     * @brief A key identifying the thread local storage (TLS) slot holding the
     *   local data of a thread.
     */
    TLS_KEY m_tlsKey;
  public: // Constructors
    /**
     * Constructs a ThreadLocalData object.
     */
    ThreadLocalData() : m_tlsKey(PIN_CreateThreadDataKey(free))
    {
      // Automatically initialise the data when a thread starts
      addThreadInitFunction(init, &m_tlsKey);
    }

  public: // Destructors
    /**
     * Destroys a ThreadLocalData object.
     */
    ~ThreadLocalData()
    {
      PIN_DeleteThreadDataKey(m_tlsKey);
    }

  private: // Internal functions for initialising and releasing data
    /**
     * Initialises local data of a thread.
     *
     * @param tid A number identifying the thread.
     * @param data A pointer to a key identifying the thread local storage (TLS)
     *   slot holding the local data of the thread.
     */
    static VOID init(THREADID tid, VOID* data)
    {
      PIN_SetThreadData(*static_cast< TLS_KEY* >(data), new T(), tid);
    }

    /**
     * Frees local data of a thread.
     *
     * @param data A pointer to local data.
     */
    static VOID free(VOID* data)
    {
      delete static_cast< T* >(data);
    }

  public: // Methods for accessing the data
    /**
     * Gets local data of a thread.
     *
     * @param tid A number identifying the thread.
     * @return A pointer to local data of the thread.
     */
    inline
    T* get(THREADID tid)
    {
      return static_cast< T* >(PIN_GetThreadData(m_tlsKey, tid));
    }
};

#endif /* __PINTOOL_ANACONDA__UTILS__TLDATA_HPP__ */

/** End of file tldata.hpp **/
