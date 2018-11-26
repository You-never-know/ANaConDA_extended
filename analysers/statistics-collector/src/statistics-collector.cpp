/*
 * Copyright (C) 2017-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief An analyser collecting various statistics about the execution.
 *
 * A file containing implementation of callback functions required to collect
 *   various statistics about a concrete execution of the analysed program.
 *
 * @file      statistics-collector.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2017-05-19
 * @date      Last Update 2017-05-22
 * @version   0.1.0.1
 */

#include "anaconda.h"

#include <map>
#include <stack>

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    /**
     * @brief A structure holding memory operation statistics.
     */
    struct MemoryOperations_s
    {
      std::map< std::string, UINT64 > all; //!< All memory operations.
      std::stack< UINT64 * > active; //!< Active memory operations.
    } memops;

    /**
     * Constructs a ThreadData_s object.
     */
    ThreadData_s() {}
  } ThreadData;

  // A key for accessing private data of a thread in the Thread Local Storage
  TLS_KEY g_tlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< ThreadData* >(data); }
  );
}

// A helper macro for accessing the Thread Local Storage (TLS) more easily
#define TLS static_cast< ThreadData* >(TLS_GetThreadData(g_tlsKey, tid))

/**
 * Updates information about the number of memory operations performed.
 *
 * @param tid A thread which performed the read.
 * @param addr An address from which were the data read.
 * @param size A size in bytes of the data read.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  // Update the number of memory operations performed in the current function
  ++(*TLS->memops.active.top());
}

/**
 * Updates information about the number of memory operations performed.
 *
 * @param tid A thread which performed the read.
 * @param addr An address from which were the data read.
 * @param size A size in bytes of the data read.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  // Update the number of memory operations performed in the current function
  ++(*TLS->memops.active.top());
}

/**
 * Updates information about the number of memory operations performed.
 *
 * @param tid A thread which performed the atomic update.
 * @param addr An address at which were the data atomically updated.
 * @param size A size in bytes of the data atomically updated.
 */
VOID beforeAtomicUpdate(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable)
{
  // Update the number of memory operations performed in the current function
  (*TLS->memops.active.top()) += 2;
}

/**
 * Updates information about the currently executed function.
 *
 * @param tid A number identifying the thread executing the function.
 */
VOID functionEntered(THREADID tid)
{
  // Helper variables
  std::string function;

  // Get the name of the function we are entering
  THREAD_GetCurrentFunction(tid, function);

  // Find the counter holding the number of memory operations performed by the
  // function we are entering (if the counter does not exist, create a new one)
  TLS->memops.active.push(&TLS->memops.all[function]);
}

/**
 * Updates information about the currently executed function.
 *
 * @param tid A number identifying the thread executing the function.
 */
VOID functionExited(THREADID tid)
{
  // Move to the counter for the parent function
  TLS->memops.active.pop();
}

/**
 * Initialises thread local storage (TLS).
 *
 * @param tid A number identifying the thread owning the TLS.
 */
VOID threadStarted(THREADID tid)
{
  // Initialise thread local storage
  TLS_SetThreadData(g_tlsKey, new ThreadData(), tid);

  // Some memory operations at the beginning may not belong to any function
  TLS->memops.active.push(&TLS->memops.all["<none>"]);
}

/**
 * Prints the collected statistics.
 *
 * @param tid A number identifying the thread for which the statistics were
 *   collected.
 */
VOID threadFinished(THREADID tid)
{
  // Helper variables
  std::map< std::string, UINT64 >::iterator it;

  CONSOLE("Statistics\n");
  CONSOLE("----------\n");

  for (it = TLS->memops.all.begin(); it != TLS->memops.all.end(); ++it)
  { // Print the number of memory operations performed by each function
    CONSOLE(it->first + ": " + decstr(it->second) + "\n");
  }
}

/**
 * Initialises the statistics collector plugin.
 */
PLUGIN_INIT_FUNCTION()
{
  ACCESS_BeforeMemoryRead(beforeMemoryRead);
  ACCESS_BeforeMemoryWrite(beforeMemoryWrite);
  ACCESS_BeforeAtomicUpdate(beforeAtomicUpdate);

  THREAD_FunctionEntered(functionEntered);
  THREAD_FunctionExited(functionExited);

  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);
}

/** End of file statistics-collector.cpp **/
