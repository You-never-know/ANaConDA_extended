/**
 * @brief An analyser performing dynamic validation of data.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of data.
 *
 * @file      data-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-07-13
 * @date      Last Update 2016-07-13
 * @version   0.1
 */

#include <map>

#include "anaconda.h"

#include "utils/lockobj.hpp"

//  0+: Attempts to lock/unlock a destroyed lock
//  1+: Attempts to lock/unlock an uninitialised lock
// 10+: Executed monitored functions
#define VERBOSITY_LEVEL 0

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief An enumeration of possible states of a lock.
   */
  typedef enum LockState_e
  {
    NOT_INITIALISED, //!< Lock is not initialised yet.
    INITIALISED,     //!< Lock is initialised.
    DESTROYED        //!< Lock is destroyed.
  } LockState;

  /**
   * @brief A structure containing information about a lock.
   */
  typedef struct LockInfo_s
  {
    LockState state; //!< A state of the lock.
    /**
     * @brief A number identifying the last thread which performed an operation
     *   on the lock.
     */
    THREADID tid;
  } LockInfo;

  class LockInfoMap : public RWLockableObject
  {
    private: // Internal variables
      std::map< ADDRINT, LockInfo > m_map;
    public:
      LockInfo& acquireLock(ADDRINT lock)
      {
        this->writelock();

        return m_map[lock];
      }

      void releaseLock(ADDRINT lock)
      {
        this->unlock();
      }
  };

  LockInfoMap g_lockInfoMap;
}

VOID onMutexInit(THREADID tid, ADDRINT* arg)
{
#if VERBOSITY_LEVEL >= 10
  CONSOLE("Lock " + hexstr(*arg) + " initialised.\n");
#endif

  LockInfo& li = g_lockInfoMap.acquireLock(*arg);

  li.state = INITIALISED;
  li.tid = tid;

  g_lockInfoMap.releaseLock(*arg);
}

VOID onMutexUse(THREADID tid, ADDRINT* arg, const std::string& op)
{
  LockInfo& li = g_lockInfoMap.acquireLock(*arg);

  switch (li.state)
  {
    case NOT_INITIALISED:
#if VERBOSITY_LEVEL >= 1
      CONSOLE("error: thread " + decstr(tid) + " is trying to " + op
        + " a lock " + hexstr(*arg) + " which was not initialised yet!\n");
#endif
      break;
    case DESTROYED:
      CONSOLE("error: thread " + decstr(tid) + " is trying to " + op
        + " a lock " + hexstr(*arg) + " which was already destroyed by thread "
        + decstr(li.tid) + "!\n");
      break;
    default:
      break;
  }

  g_lockInfoMap.releaseLock(*arg);
}

VOID onMutexLock(THREADID tid, ADDRINT* arg)
{
#if VERBOSITY_LEVEL >= 10
  CONSOLE("Lock " + hexstr(*arg) + " acquired.\n");
#endif

  onMutexUse(tid, arg, "acquire");
}

VOID onMutexUnlock(THREADID tid, ADDRINT* arg)
{
#if VERBOSITY_LEVEL >= 10
  CONSOLE("Lock " + hexstr(*arg) + " released.\n");
#endif

  onMutexUse(tid, arg, "release");
}

VOID onMutexDestroy(THREADID tid, ADDRINT* arg)
{
#if VERBOSITY_LEVEL >= 10
  CONSOLE("Lock " + hexstr(*arg) + " destroyed.\n");
#endif

  LockInfo& li = g_lockInfoMap.acquireLock(*arg);

  li.state = DESTROYED;
  li.tid = tid;

  g_lockInfoMap.releaseLock(*arg);
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Register callback functions called when a function is executed
  THREAD_FunctionExecuted("pthread_mutex_init", onMutexInit, 1);
  THREAD_FunctionExecuted("__pthread_mutex_lock", onMutexLock, 1);
  THREAD_FunctionExecuted("__pthread_mutex_unlock_usercnt", onMutexUnlock, 1);
  THREAD_FunctionExecuted("__pthread_mutex_destroy", onMutexDestroy, 1);
}

/** End of file data-validator.cpp **/
