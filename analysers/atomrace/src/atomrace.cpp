/**
 * @brief Contains the entry part of the AtomRace ANaConDA plugin.
 *
 * A file containing the entry part of the AtomRace ANaConDA plugin.
 *
 * @file      atomrace.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-30
 * @date      Last Update 2012-04-07
 * @version   0.2.2
 */

#include "anaconda.h"

#include <map>

/**
 * @brief An enumeration describing access operations.
 */
typedef enum Operation_e
{
  READ, //!< A read operation.
  WRITE //!< A write operation.
} Operation;

/**
 * @brief A structure holding information about a currently accessed part of
 *   a memory.
 */
typedef struct CurrentAccess_s
{
  Operation op; //!< A type of the access.
  THREADID thread; //!< A thread which is performing the access.
  VARIABLE variable; //!< A variable which is accessed.
  /**
   * @brief A source code location where the access originates from.
   */
  LOCATION location;

  /**
   * Constructs a CurrentAccess_s object.
   */
  CurrentAccess_s() : op(READ), thread(0), variable(), location() {}

  /**
   * Constructs a CurrentAccess_s object.
   *
   * @param o A type of the access.
   * @param t A thread which is performing the access.
   * @param v A variable which is accessed.
   * @param l A source code location where the access originates from.
   */
  CurrentAccess_s(Operation o, THREADID t, VARIABLE v, LOCATION l) : op(o),
    thread(t), variable(v), location(l) {}
} CurrentAccess;

namespace
{ // Static global variables (usable only within this module)
  typedef std::map< ADDRINT, CurrentAccess > CurrentAccessMap;

  /**
   * @brief A map containing current accesses to the memory.
   */
  CurrentAccessMap g_currentAccessMap;
  /**
   * @brief A mutex guarding accesses to the @em g_currentAccessMap.
   */
  PIN_MUTEX g_currentAccessMapMutex;
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
 * Checks if an access to a memory is causing a data race.
 *
 * @param op A type of the access.
 * @param tid A number uniquely identifying a thread which is performing the
 *   access.
 * @param addr An address which is accessed.
 * @param variable A variable which is accessed.
 * @param location A source code location where the access originates from.
 */
VOID beforeMemoryAccess(Operation op, THREADID tid, ADDRINT addr,
  const VARIABLE& variable, const LOCATION& location)
{
  // Accesses to current access map must be exclusive
  PIN_MutexLock(&g_currentAccessMapMutex);

  // Check if some other thread is accessing the same memory address
  CurrentAccessMap::iterator it = g_currentAccessMap.find(addr);

  if (it != g_currentAccessMap.end())
  { // Some other thread is accessing the same memory address (no need to check
    // if the threads are different, they must be)
    if (it->second.op == WRITE || op == WRITE)
    { // One of the concurrent accesses is a write access, report a data race
      CONSOLE_NOPREFIX("Data race on memory address " + hexstr(addr)
        + " detected.\n"
        + "  Thread " + decstr(it->second.thread)
        + ((it->second.op == WRITE) ? " written to " : " read from ")
        + getVariableDeclaration(it->second.variable) + "\n"
        + "    accessed at line " + decstr(it->second.location.line)
        + " in file " + ((it->second.location.file.empty()) ?
          "<unknown>" : it->second.location.file) + "\n"
        + "  Thread " + decstr(tid)
        + ((op == WRITE) ? " written to " : " read from ")
        + getVariableDeclaration(variable) + "\n"
        + "    accessed at line " + decstr(location.line) + " in file "
        + ((location.file.empty()) ? "<unknown>" : location.file) + "\n");

      // Helper variables
      Backtrace bt;
      Symbols symbols;

      // Get the backtrace of the current thread
      THREAD_GetBacktrace(tid, bt);
      // Translate the return addresses to locations
      THREAD_GetBacktraceSymbols(bt, symbols);

      CONSOLE_NOPREFIX("\n  Thread " + decstr(tid) + " backtrace:\n");

      for (Symbols::size_type i = 0; i < symbols.size(); i++)
      { // Print information about each return address in the backtrace
        CONSOLE_NOPREFIX("    #" + decstr(i) + (i > 10 ? " " : "  ")
          + symbols[i] + "\n");
      }

      CONSOLE_NOPREFIX("\n");
    }
  }
  else
  { // If no thread is currently accessing the memory, record this access
    g_currentAccessMap.insert(CurrentAccessMap::value_type(addr,
      CurrentAccess(op, tid, variable, location)));
  }

  // Now we can finally release the lock
  PIN_MutexUnlock(&g_currentAccessMapMutex);
}

/**
 * Removes information about an access to a memory.
 *
 * @param op A type of the access.
 * @param tid A number uniquely identifying a thread which is performing the
 *   access.
 * @param addr An address which is accessed.
 * @param variable A variable which is accessed.
 * @param location A source code location where the access originates from.
 */
VOID afterMemoryAccess(Operation op, THREADID tid, ADDRINT addr,
  const VARIABLE& variable, const LOCATION& location)
{
  // Accesses to current access map must be exclusive
  PIN_MutexLock(&g_currentAccessMapMutex);

  // Check if some thread is currently accessing the same memory address
  CurrentAccessMap::iterator it = g_currentAccessMap.find(addr);

  if (it != g_currentAccessMap.end() && it->second.thread == tid)
  { // Its this thread which is accessing the address, record the access end
    g_currentAccessMap.erase(it);
  }

  // Now we can finally release the lock
  PIN_MutexUnlock(&g_currentAccessMapMutex);
}

/**
 * Checks if a read from a memory is causing a data race.
 *
 * @param tid A thread which is performing the read.
 * @param addr An address from which are the data read.
 * @param size A size in bytes of the data read.
 * @param variable A structure containing information about a variable stored
 *   at the address from which are the data read.
 * @param location A source code location where the read originates from.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  beforeMemoryAccess(READ, tid, addr, variable, location);
}

/**
 * Checks if a write to a memory is causing a data race.
 *
 * @param tid A thread which is performing the write.
 * @param addr An address to which are the data written.
 * @param size A size in bytes of the data written.
 * @param variable A structure containing information about a variable stored
 *   at the address to which are the data written.
 * @param location A source code location where the write originates from.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  beforeMemoryAccess(WRITE, tid, addr, variable, location);
}

/**
 * Removes information about a read from a memory.
 *
 * @param tid A thread which is performing the read.
 * @param addr An address from which are the data read.
 * @param size A size in bytes of the data read.
 * @param variable A structure containing information about a variable stored
 *   at the address from which are the data read.
 * @param location A source code location where the read originates from.
 */
VOID afterMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  afterMemoryAccess(READ, tid, addr, variable, location);
}

/**
 * Removes information about a write to a memory.
 *
 * @param tid A thread which is performing the write.
 * @param addr An address to which are the data written.
 * @param size A size in bytes of the data written.
 * @param variable A structure containing information about a variable stored
 *   at the address to which are the data written.
 * @param location A source code location where the write originates from.
 */
VOID afterMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  afterMemoryAccess(WRITE, tid, addr, variable, location);
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

  // Register callback functions called after access events
  ACCESS_AfterMemoryRead(afterMemoryRead);
  ACCESS_AfterMemoryWrite(afterMemoryWrite);

  // Initialise R/W mutex for guarding access to the last access map
  PIN_MutexInit(&g_currentAccessMapMutex);
}

/** End of file atomrace.cpp **/
