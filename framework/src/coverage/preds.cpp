/**
 * @brief Contains implementation of classes monitoring predecessors.
 *
 * A file containing implementation of classes monitoring predecessors.
 *
 * @file      preds.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-04-05
 * @date      Last Update 2013-04-16
 * @version   0.1
 */

#include "preds.h"

#include <list>

#include "../callbacks/thread.h"

#include "../util/scopedlock.hpp"
#include "../util/writers.h"

// Helper macros
#define THREAD_DATA getThreadData(tid)

// Declarations of static functions (usable only within this module)
static VOID deleteThreadData(void* threadData);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_threadDataTlsKey = PIN_CreateThreadDataKey(deleteThreadData);
}

/**
 * @brief A structure holding private data of a thread.
 */
typedef struct ThreadData_s
{
  /**
   * @brief A set of variables accessed by a thread in a single function.
   */
  std::list< std::set< std::string > > vars;

  /**
   * Constructs a ThreadData_s object.
   */
  ThreadData_s() : vars()
  {
    vars.push_back(std::set< std::string >());
  }
} ThreadData;

/**
 * Deletes an object holding private data of a thread.
 *
 * @param threadData An object holding private data of a thread.
 */
VOID deleteThreadData(void* threadData)
{
  delete static_cast< ThreadData* >(threadData);
}

/**
 * Gets an object holding private data of a thread.
 *
 * @param tid A number identifying the thread.
 * @return An object holding private data of the thread.
 */
inline
ThreadData* getThreadData(THREADID tid)
{
  return static_cast< ThreadData* >(PIN_GetThreadData(g_threadDataTlsKey, tid));
}

/**
 * Destroys a PredecessorsMonitor object and writes all predecessors detected
 *   to a file.
 */
template< typename Writer >
PredecessorsMonitor< Writer >::~PredecessorsMonitor()
{
  PIN_RWMutexFini(&m_pSetLock);

  for (PredecessorSet::iterator it = m_pSet.begin(); it != m_pSet.end(); it++)
  { // Write all predecessors to output file
    this->writeln(hexstr(*it));
  }
}

/**
 * Initialises TLS (thread local storage) data for a thread.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
template< typename Writer >
VOID PredecessorsMonitor< Writer >::initTls(THREADID tid, CONTEXT* ctxt,
  INT32 flags, VOID* v)
{
  // Allocate memory for storing private data of the starting thread
  PIN_SetThreadData(g_threadDataTlsKey, new ThreadData(), tid);
}

/**
 * Loads instructions with predecessors from a file.
 *
 * @note This method does not check the existence of the file, if required, it
 *   should be done before calling this method.
 *
 * @param path A path to a file containing instructions with predecessors.
 */
template< typename Writer >
void PredecessorsMonitor< Writer >::load(const std::string& path)
{
  // Extract info about all instructions with predecessors from a previous run
  std::fstream f(path, std::fstream::in);

  // Helper variables
  std::string line;

  while (std::getline(f, line) && !f.fail())
  { // Each line contains the address of one instruction with predecessor
    if (line.empty()) continue;

    m_pSet.insert(AddrintFromString(line));
  }
}

/**
 * Creates an empty set of variables accessed in a function.
 *
 * @note This method is called before a thread enters a function.
 *
 * @param tid A thread entering a function.
 */
template< typename Writer >
void PredecessorsMonitor< Writer >::beforeFunctionEntered(THREADID tid)
{
  THREAD_DATA->vars.push_back(std::set< std::string >());
}

/**
 * Deletes a set of variables accessed in a function.
 *
 * @note This method is called before a thread leaves a function.
 *
 * @param tid A thread leaving a function.
 */
template< typename Writer >
void PredecessorsMonitor< Writer >::beforeFunctionExited(THREADID tid)
{
  THREAD_DATA->vars.pop_back();
}

/**
 * Updates a set of instructions with predecessors.
 *
 * @note This method is called before a thread accesses a variable.
 *
 * @param tid A thread accessing a variable.
 * @param ins An address of an instruction accessing a variable.
 * @param var A variable accessed by a thread.
 */
template< typename Writer >
void PredecessorsMonitor< Writer >::beforeVariableAccessed(THREADID tid,
  ADDRINT ins, VARIABLE var)
{
  std::set< std::string >& vSet = THREAD_DATA->vars.back();

  if (vSet.find(var.name) != vSet.end())
  { // This variable was accessed before
    ScopedWriteLock wrtlock(m_pSetLock);

    m_pSet.insert(ins); // The instruction has a predecessor
  }
  else
  { // This variable was not accessed before, remember it
    vSet.insert(var.name);
  }
}

/**
 * Checks if an instruction has a predecessor.
 *
 * @param ins An address of an instruction.
 * @return @em True if the instruction has a predecessor, @em false otherwise.
 */
template< typename Writer >
bool PredecessorsMonitor< Writer >::hasPredecessor(ADDRINT ins)
{
  // Other threads might be reading from the map with us, no problem
  ScopedReadLock rdlock(m_pSetLock);

  // If the instruction is in the predecessor set, it has a predecessor
  return m_pSet.find(ins) != m_pSet.end();
}

// Instantiate the monitor for various writers
template class PredecessorsMonitor< FileWriter >;

/** End of file preds.cpp **/
