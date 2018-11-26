/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains the entry part of the event printer ANaConDA plugin.
 *
 * A file containing the entry part of the event printer ANaConDA plugin.
 *
 * @file      event-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-05
 * @date      Last Update 2016-03-31
 * @version   0.2
 */

#include "anaconda.h"

#include "utils/plugin/settings.hpp"

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
 * Prints information about a read from a memory.
 *
 * @param tid A thread which performed the read.
 * @param addr An address from which were the data read.
 * @param size A size in bytes of the data read.
 * @param variable A structure containing information about a variable stored
 *   at the address from which were the data read.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("Before thread " + decstr(tid)
    + " read " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " from memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
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
VOID afterMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("After thread " + decstr(tid)
    + " read " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " from memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
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
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("Before thread " + decstr(tid)
    + " written " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " to memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
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
VOID afterMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("After thread " + decstr(tid)
    + " written " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " to memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
}

/**
 * Prints information about an atomic update of a memory.
 *
 * @param tid A thread which performed the atomic update.
 * @param addr An address at which were the data atomically updated.
 * @param size A size in bytes of the data atomically updated.
 * @param variable A structure containing information about a variable stored
 *   at the address at which were the data atomically updated.
 */
VOID beforeAtomicUpdate(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("Before thread " + decstr(tid)
    + " updated " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " at memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
}

/**
 * Prints information about an atomic update of a memory.
 *
 * @param tid A thread which performed the atomic update.
 * @param addr An address at which were the data atomically updated.
 * @param size A size in bytes of the data atomically updated.
 * @param variable A structure containing information about a variable stored
 *   at the address at which were the data atomically updated.
 */
VOID afterAtomicUpdate(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location)
{
  CONSOLE("After thread " + decstr(tid)
    + " updated " + decstr(size) + " " + ((size == 1) ? "byte" : "bytes")
    + " at memory address " + hexstr(addr)
    + "\n  variable " + getVariableDeclaration(variable)
    + "\n  accessed at line " + decstr(location.line)
    + " in file " + ((location.file.empty()) ? "<unknown>" : location.file)
    + "\n");
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  CONSOLE("Before lock acquired: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  CONSOLE("Before lock released: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID beforeSignal(THREADID tid, COND cond)
{
  CONSOLE("Before signal send: thread " + decstr(tid) + ", condition " + cond
    + "\n");
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param cond An object representing the condition on which is the thread
 *   waiting.
 */
VOID beforeWait(THREADID tid, COND cond)
{
  CONSOLE("Before wait: thread " + decstr(tid) + ", condition " + cond + "\n");
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  CONSOLE("After lock acquired: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID afterLockRelease(THREADID tid, LOCK lock)
{
  CONSOLE("After lock released: thread " + decstr(tid) + ", lock " + lock
    + "\n");
}

/**
 * Prints information about a condition signalled.
 *
 * @param tid A thread from which was the condition signalled.
 * @param cond An object representing the condition signalled.
 */
VOID afterSignal(THREADID tid, COND cond)
{
  CONSOLE("After signal send: thread " + decstr(tid) + ", condition " + cond
    + "\n");
}

/**
 * Prints information about a condition on which a thread is waiting.
 *
 * @param tid A thread which is waiting on the condition.
 * @param cond An object representing the condition on which is the thread
 *   waiting.
 */
VOID afterWait(THREADID tid, COND cond)
{
  CONSOLE("After wait: thread " + decstr(tid) + ", condition " + cond + "\n");
}

/**
 * Prints information about a thread which is about to start.
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  CONSOLE("Thread " + decstr(tid) + " started.\n");
}

/**
 * Prints information about a thread which is about to finish.
 *
 * @param tid A number identifying the thread.
 */
VOID threadFinished(THREADID tid)
{
  CONSOLE("Thread " + decstr(tid) + " finished.\n");
}

/**
 * Prints information about a function to be executed by a specific thread.
 *
 * @param tid A number identifying the thread.
 */
VOID functionEntered(THREADID tid)
{
  // Helper variables
  std::string signature;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, signature);

  CONSOLE("Thread " + decstr(tid) + " started executing a function "
    + signature + "\n");
}

/**
 * Prints information about a function that was executed by a specific thread.
 *
 * @param tid A number identifying the thread.
 */
VOID functionExited(THREADID tid)
{
  // Helper variables
  std::string signature;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, signature);

  CONSOLE("Thread " + decstr(tid) + " finished executing a function "
    + signature + "\n");
}

/**
 * Prints information about the threads joining together.
 *
 * @param tid A number identifying the thread which wants to join with another
 *   thread.
 * @param jtid A number identifying the thread which is about to be joined with
 *   the first thread.
 */
VOID beforeJoin(THREADID tid, THREADID jtid)
{
  CONSOLE("Before thread " + decstr(tid) + " joined with thread " + decstr(jtid)
    + "\n");
}

/**
 * Prints information about the threads joining together.
 *
 * @param tid A number identifying the thread which wanted to join with another
 *   thread.
 * @param jtid A number identifying the thread which is was joined with the
 *   first thread.
 */
VOID afterJoin(THREADID tid, THREADID jtid)
{
  CONSOLE("After thread " + decstr(tid) + " joined with thread " + decstr(jtid)
    + "\n");
}

/**
 * Prints information about an exception thrown by a thread.
 *
 * @param tid A number identifying the thread.
 * @param exception An object representing the exception which was thrown.
 */
VOID exceptionThrown(THREADID tid, const EXCEPTION& exception)
{
  CONSOLE("Thread " + decstr(tid) + " has thrown exception " + exception.name
    + ".\n");
}

/**
 * Prints information about an exception caught by a thread.
 *
 * @param tid A number identifying the thread.
 * @param exception An object representing the exception which was caught.
 */
VOID exceptionCaught(THREADID tid, const EXCEPTION& exception)
{
  CONSOLE("Thread " + decstr(tid) + " has caught exception " + exception.name
    + ".\n");
}

/**
 * Initialises the event printer plugin.
 */
PLUGIN_INIT_FUNCTION()
{
  // Helper variables
  Settings settings;

  // Register all settings supported by the analyser
  settings.addOptions()
    FLAG("monitor.access.reads", true)
    FLAG("monitor.access.writes", true)
    FLAG("monitor.access.updates", true)
    FLAG("monitor.sync.acquires", true)
    FLAG("monitor.sync.releases", true)
    FLAG("monitor.sync.signals", true)
    FLAG("monitor.sync.waits", true)
    FLAG("monitor.sync.joins", true)
    FLAG("monitor.thread.starts", true)
    FLAG("monitor.thread.ends", true)
    FLAG("monitor.function.enters", true)
    FLAG("monitor.function.exits", true)
    FLAG("monitor.exception.throws", true)
    FLAG("monitor.exception.catches", true)
    ;

  // Load plugin's settings, continue on error
  LOAD_SETTINGS(settings, "event-printer.conf");

  // Helper macros
  #define ENABLED(flag) settings.enabled(flag)

  // Register callback functions called before access events
  if (ENABLED("monitor.access.reads"))
    ACCESS_BeforeMemoryRead(beforeMemoryRead);
  if (ENABLED("monitor.access.writes"))
    ACCESS_BeforeMemoryWrite(beforeMemoryWrite);
  if (ENABLED("monitor.access.updates"))
    ACCESS_BeforeAtomicUpdate(beforeAtomicUpdate);

  // Register callback functions called after access events
  if (ENABLED("monitor.access.reads"))
    ACCESS_AfterMemoryRead(afterMemoryRead);
  if (ENABLED("monitor.access.writes"))
    ACCESS_AfterMemoryWrite(afterMemoryWrite);
  if (ENABLED("monitor.access.updates"))
    ACCESS_AfterAtomicUpdate(afterAtomicUpdate);

  // Register callback functions called before synchronisation events
  if (ENABLED("monitor.sync.acquires"))
    SYNC_BeforeLockAcquire(beforeLockAcquire);
  if (ENABLED("monitor.sync.releases"))
    SYNC_BeforeLockRelease(beforeLockRelease);
  if (ENABLED("monitor.sync.signals"))
    SYNC_BeforeSignal(beforeSignal);
  if (ENABLED("monitor.sync.waits"))
    SYNC_BeforeWait(beforeWait);
  if (ENABLED("monitor.sync.joins"))
    SYNC_BeforeJoin(beforeJoin);

  // Register callback functions called after synchronisation events
  if (ENABLED("monitor.sync.acquires"))
    SYNC_AfterLockAcquire(afterLockAcquire);
  if (ENABLED("monitor.sync.releases"))
    SYNC_AfterLockRelease(afterLockRelease);
  if (ENABLED("monitor.sync.signals"))
    SYNC_AfterSignal(afterSignal);
  if (ENABLED("monitor.sync.waits"))
    SYNC_AfterWait(afterWait);
  if (ENABLED("monitor.sync.joins"))
    SYNC_AfterJoin(afterJoin);

  // Register callback functions called when a thread starts or finishes
  if (ENABLED("monitor.thread.starts"))
    THREAD_ThreadStarted(threadStarted);
  if (ENABLED("monitor.thread.ends"))
    THREAD_ThreadFinished(threadFinished);

  // Register callback functions called when a function is executed
  if (ENABLED("monitor.function.enters"))
    THREAD_FunctionEntered(functionEntered);
  if (ENABLED("monitor.function.exits"))
    THREAD_FunctionExited(functionExited);

  // Register callback functions called when an exception is thrown or caught
  if (ENABLED("monitor.exception.throws"))
    EXCEPTION_ExceptionThrown(exceptionThrown);
  if (ENABLED("monitor.exception.catches"))
    EXCEPTION_ExceptionCaught(exceptionCaught);
}

/** End of file event-printer.cpp **/
