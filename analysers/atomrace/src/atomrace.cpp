/**
 * @brief Contains the entry part of the AtomRace ANaConDA plugin.
 *
 * A file containing the entry part of the AtomRace ANaConDA plugin.
 *
 * @file      atomrace.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-30
 * @date      Last Update 2012-02-04
 * @version   0.1
 */

#include "anaconda.h"

#include <map>
#include <set>

// Type definitions
typedef std::set< LOCK > LockSet;

/**
 * @brief An enumeration describing access operations.
 */
typedef enum Operation_e
{
  READ, //!< A read operation.
  WRITE //!< A write operation.
} Operation;

/**
 * @brief A structure holding information about the last access to a memory
 *   location.
 */
typedef struct LastAccess_s
{
  Operation op; //!< An access operation.
  THREADID thread; //!< A thread which performed the access.
  VARIABLE variable; //!< A variable which was accessed.
  LockSet lockset; //!< A lock set of the thread which performed the access.

  /**
   * Constructs a LastAccess_s object.
   */
  LastAccess_s() : op(READ), thread(0), variable(), lockset() {}

  /**
   * Constructs a LastAccess_s object.
   *
   * @param o An access operation.
   * @param tid A thread which performed the access.
   * @param v A variable which was accessed.
   * @param ls A lock set of the thread which performed the access.
   */
  LastAccess_s(Operation o, THREADID tid, VARIABLE v, LockSet& ls) : op(o),
    thread(tid), variable(v), lockset(ls) {}
} LastAccess;

// Declarations of static functions (usable only within this module)
static VOID deleteLockSet(void* lset);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_lockSetTlsKey = TLS_CreateThreadDataKey(deleteLockSet);

  // Type definitions
  typedef std::map< ADDRINT, LastAccess > LastAccessMap;

  /**
   * @brief A map containing last accesses to various memory locations.
   */
  LastAccessMap g_lastAccessMap;
  /**
   * @brief A mutex guarding accesses to the @em g_lastAccessMap.
   */
  PIN_MUTEX g_lastAccessMapMutex;
}

/**
 * Deletes a lock set created during thread start.
 *
 * @param lset A lock set.
 */
VOID deleteLockSet(void* lset)
{
  delete static_cast< LockSet* >(lset);
}

/**
 * Gets a lock set associated with a specific thread.
 *
 * @param tid A number uniquely identifying the thread.
 * @return The lock set associated with the specified thread.
 */
inline
LockSet* getLockSet(THREADID tid)
{
  return static_cast< LockSet* >(TLS_GetThreadData(g_lockSetTlsKey, tid));
}

/**
 * Checks if two lock sets contain at least one same lock.
 *
 * @param ls1 The first lock set.
 * @param ls2 The secong lock set.
 * @return @em True if the lock sets contain at least one same lock.
 */
inline
bool containSameLock(const LockSet& ls1, const LockSet& ls2)
{
  for (LockSet::iterator it = ls1.begin(); it != ls1.end(); it++)
  { // Check if some of the locks in the first set is also in the second set
    if (ls2.count(*it) > 0) return true;
  }

  // The two set are disjunctive
  return false;
}

/**
 * Gets a declaration of a variable.
 *
 * @param variable A structure containing the information about the variable.
 * @return A string containing the declaration of the variable.
 */
inline
std::string getVariableDeclaration(const VARIABLE& variable)
{
  // Format the name, type and offset to a 'type name[+offset]' string
  return ((variable.type.size() == 0) ? "" : variable.type + " ")
    + ((variable.name.empty()) ? "<unknown>" : variable.name)
    + ((variable.offset == 0) ? "" : "+" + decstr(variable.offset));
}

/**
 * Checks if an access to a memory location should cause a data race.
 *
 * @param op An access operation.
 * @param tid A number uniquely identifying a thread which performed the access.
 * @param addr An address accessed.
 * @param variable A variable accessed.
 */
VOID beforeMemoryAccess(Operation op, THREADID tid, ADDRINT addr,
  const VARIABLE& variable)
{
  // Accesses to the last access map must be exclusive
  PIN_MutexLock(&g_lastAccessMapMutex);

  // Try to find the last access to the specified memory location
  LastAccessMap::iterator it = g_lastAccessMap.find(addr);

  if (it != g_lastAccessMap.end())
  { // Memory access before, check if the new access will not cause a data race
    if (it->second.thread != tid && it->second.thread != 0 && tid != 0)
    { // Only accesses from different threads might cause a data race
      // TODO: Do not ignore thread 0 which causes false alarms
      if (it->second.op == WRITE || op == WRITE)
      { // One of the access operations must be a write operation
        if (!containSameLock(it->second.lockset, *getLockSet(tid)))
        { // If the accesses are not guarded by the same lock, it is a data race
          CONSOLE(std::string("Data race detected.\n")
            + "Thread " + decstr(it->second.thread)
            + ((it->second.op == WRITE) ? " written to " : " read from ")
            + getVariableDeclaration(it->second.variable) + "\n"
            + "Thread " + decstr(tid)
            + ((op == WRITE) ? " written to " : " read from ")
            + getVariableDeclaration(variable) + "\n");
        }
      }
    }
  }

  // Save the new access as the last one
  g_lastAccessMap[addr] = LastAccess(op, tid, variable, *getLockSet(tid));

  // Now we can finally release the lock
  PIN_MutexUnlock(&g_lastAccessMapMutex);
}

/**
 * Prints information about a read from a memory.
 *
 * @param tid A thread which performed the read.
 * @param addr An address from which were the data read.
 * @param size A size in bytes of the data read.
 * @param variable A structure containing information about a variable stored
 *   at the address from which were the data read.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  beforeMemoryAccess(READ, tid, addr, variable);
}

/**
 * Prints information about a write to a memory.
 *
 * @param tid A thread which performed the write.
 * @param addr An address to which were the data written.
 * @param size A size in bytes of the data written.
 * @param variable A structure containing information about a variable stored
 *   at the address to which were the data written.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  beforeMemoryAccess(WRITE, tid, addr, variable);
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  // Add the acquired lock to the lock set of the thread
  getLockSet(tid)->insert(lock);
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  // Remove the released lock from the lock set of the thread
  getLockSet(tid)->erase(lock);
}

/**
 * Prints information about a thread which is about to start.
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  // Initialise a lock set of the newly created thread
  TLS_SetThreadData(g_lockSetTlsKey, new LockSet(), tid);
}

/**
 * Initialises the event printer plugin.
 */
extern "C"
void init()
{
  // Register callback functions called before access events
  ACCESS_BeforeMemoryRead(beforeMemoryRead);
  ACCESS_BeforeMemoryWrite(beforeMemoryWrite);

  // Register callback functions called before synchronisation events
  SYNC_BeforeLockRelease(beforeLockRelease);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);

  // Initialise R/W mutex for guarding access to the last access map
  PIN_MutexInit(&g_lastAccessMapMutex);
}

/** End of file atomrace.cpp **/
