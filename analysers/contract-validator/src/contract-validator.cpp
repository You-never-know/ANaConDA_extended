/*
 * Copyright (C) 2016-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief An analyser performing dynamic validation of contracts.
 *
 * A file containing implementation of callback functions required to obtain
 *   the information needed for performing dynamic validation of contracts.
 *
 * @file      contract-validator.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2019-02-05
 * @version   0.8.2
 */

#include "anaconda/anaconda.h"

#include <string>
#include <vector>

#include <boost/filesystem/fstream.hpp>

#include "pin.H"

#include "contract.h"
#include "vc.hpp"
#include "window.h"

//  1+: Thread start/finish/fork/join, TID->UID mappings
// 10+: Function entered/exited
#define VERBOSITY_LEVEL 1

#define MAX_RUNNING_THREADS PIN_MAX_THREADS
#define MAX_TRACKED_THREADS PIN_MAX_THREADS * 10

// Namespace aliases
namespace fs = boost::filesystem;

namespace
{ // Internal type definitions and variables (usable only within this module)
  /**
   * @brief A list mapping currently running threads into unique IDs.
   *
   * @note The size of this list cannot exceed @c MAX_RUNNING_THREADS.
   */
  std::vector< THREADID > g_threads;
  /**
   * @brief A list of trace windows owned by some thread.
   *
   * The list contains also trace windows of threads that already finished
   *   their execution. The indices are unique IDs given by @c g_threads.
   *
   * @note The size of this list cannot exceed @c MAX_TRACKED_THREADS.
   */
  WindowList g_windows;
  PIN_MUTEX g_uidLock; //!< A lock guarding unique ID generation.

  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    Window* window; //!< A trace window kept by the thread.

    /**
     * Constructs a ThreadData_s object.
     *
     * @param tid A thread owning the data.
     */
    ThreadData_s(THREADID tid) : window(new Window(tid, g_windows)) {}
  } ThreadData;

  // A key for accessing private data of a thread in the Thread Local Storage
  TLS_KEY g_tlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< ThreadData* >(data); }
  );

  std::list< Contract* > g_contracts; //!< A list of contracts to be checked.

  typedef std::map< LOCK, VectorClock > LockVectorClocks;
  LockVectorClocks g_locks; //!< Vector clocks for locks (L).
  PIN_RWMUTEX g_locksLock; //!< A lock guarding access to @c g_locks map.
}

// A helper macro for accessing a number uniquely identifying current thread
#define UID g_threads[tid]
// A helper macro for accessing a number uniquely identifying chosen thread
#define TUID(tid) g_threads[tid]
// A helper macro for accessing the Thread Local Storage (TLS) more easily
#define TLS static_cast< ThreadData* >(TLS_GetThreadData(g_tlsKey, tid))
// A helper macro for accessing a window of other thread by its PIN's TID
#define WINDOW(tid) g_windows[g_threads[tid]]

/**
 * Gets a number uniquely identifying a thread.
 *
 * @param tid A (reusable) number identifying the thread.
 * @return A number uniquely identifying the thread.
 */
THREADID getThreadUid(THREADID tid)
{
  // Helper variables
  THREADID uid;

  while (tid > g_threads.size())
  { // Some other thread with a lower ID has started before us, but we reached
    // this place before it, we need to align the thread IDs with the vector
    // positions so we need to wait for the other thread to get here first
    PIN_Sleep(10);
  }

  if (tid <= g_threads.size())
  { // Generate a new unique ID for this thread
    PIN_MutexLock(&g_uidLock);

    // The unique ID is the first available position in the window list
    uid = g_windows.size();
    // We cannot exceed the maximum number of tracked threads supported
    assert(uid != MAX_TRACKED_THREADS);
    // This ensures that the next thread will get a different (next) ID
    g_windows.push_back(NULL);

    // Note that the other reads (not used for ID generation) can be
    // done without holding any lock
    PIN_MutexUnlock(&g_uidLock);

    if (tid == g_threads.size())
    { // This may only happen for a single thread at a time, reallocation is
      // not possible as we already preallocated all the space which means
      // we can insert an item at the end of the list without invalidating
      // any concurrent accesses
      g_threads.push_back(uid);
    }
    else if (tid < g_threads.size())
    { // The previous thread with this thread ID already ended so we are the
      // only ones accessing this position so we can do it without any lock
      g_threads[tid] = uid;
    }
    else
    { // This should not happen
      assert(false);
    }
  }

#if VERBOSITY_LEVEL >= 1
  INFO("Mapping Thread " + decstr(tid) + " into Thread " + decstr(uid) + "\n");
#endif

  return uid; // This number uniquely identifies this thread now
}

/**
 * Gets a name of a function currently being executed by a specific thread.
 *
 * @param tid A number identifying the thread.
 * @param name A name of the function currently being executed by the thread @em
 *   tid.
 * @return @em True if the name was obtained successfully, @em False if the name
 *   could not be obtained or is empty.
 */
inline
BOOL getCurrentFunctionName(THREADID tid, std::string& name)
{
  // Helper variables
  std::string signature;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, signature);

  if (signature.empty())
  { // Make sure the destination string is empty
    name.clear();

    return false;
  }

  // The obtained signature has the following format: <module>!<function>
  name = signature.substr(signature.find('!') + 1);

  return true; // Name of the function obtained successfully
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  // As only a single thread may release a specific lock at one time, different
  // threads cannot insert items with the same key concurrently, however, it is
  // possible that two threads may insert items with different keys at the same
  // time and we need to do it safely (write lock gives us exclusive access)
  PIN_RWMutexWriteLock(&g_locksLock);
  g_locks[lock] = TLS->window->cvc; // L_lock' = C_tid
  PIN_RWMutexUnlock(&g_locksLock);

  TLS->window->cvc.increment(UID); // C_tid' = inc_tid(C_tid)
}

/**
 * TODO
 *
 * @param tid A number identifying the thread which wants to join with another
 *   thread.
 * @param jtid A number identifying the thread which is about to be joined with
 *   the first thread.
 */
VOID beforeJoin(THREADID tid, THREADID jtid)
{
#if VERBOSITY_LEVEL >= 1
  CONSOLE("Before thread " + decstr(tid) + " joined with thread " + decstr(jtid)
    + "\n");
#endif

  TLS->window->cvc.join(WINDOW(jtid)->cvc); // C_tid' = C_tid join C_jtid
  WINDOW(jtid)->cvc.increment(TUID(jtid)); // C_jtid' = inc_jtid(C_jtid)
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  // We are only reading information here, so read lock is sufficient for us
  PIN_RWMutexReadLock(&g_locksLock);
  // TODO: based on the documentation, it should be safe to read other items
  // in the map concurrently with the insertion of new items, however, it is
  // still questionable as std::map uses RB trees which may need rebalancing

  // Obtain the vector clock of the lock we just acquired
  LockVectorClocks::iterator it = g_locks.find(lock);

  if (it != g_locks.end())
  { // Everything before this lock was released happened before us
    TLS->window->cvc.join(it->second); // C_tid' = C_tid join L_lock
  }

  PIN_RWMutexUnlock(&g_locksLock);
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID afterLockRelease(THREADID tid, LOCK lock)
{
  //
}

/**
 * TODO
 *
 * @param tid A number identifying the thread which wanted to join with another
 *   thread.
 * @param jtid A number identifying the thread which is was joined with the
 *   first thread.
 */
VOID afterJoin(THREADID tid, THREADID jtid)
{
#if VERBOSITY_LEVEL >= 1
  CONSOLE("After thread " + decstr(tid) + " joined with thread " + decstr(jtid)
    + "\n");
#endif
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
#if VERBOSITY_LEVEL >= 1
  CONSOLE("Thread " + decstr(tid) + " started\n");
#endif

  // Initialise thread local data (window, current vector clock, etc.)
  TLS_SetThreadData(g_tlsKey, new ThreadData(getThreadUid(tid)), tid);

  for (Contract* contract : g_contracts)
  { // Monitor all loaded contracts
    TLS->window->monitor(contract);
  }

  // Make the window visible to other threads
  g_windows[UID] = TLS->window;
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID threadFinished(THREADID tid)
{
#if VERBOSITY_LEVEL >= 1
  CONSOLE("Thread " + decstr(tid) + " finished\n");
#endif
}

/**
 * TODO
 *
 * @param tid A number identifying the thread which created a new thread.
 * @param ftid A number identifying the new thread created.
 */
VOID threadForked(THREADID tid, THREADID ftid)
{
#if VERBOSITY_LEVEL >= 1
  CONSOLE("Thread " + decstr(tid) + " forked thread " + decstr(ftid) + "\n");
#endif

  WINDOW(ftid)->cvc.join(TLS->window->cvc); // C_ftid' = C_ftid join C_tid
  TLS->window->cvc.increment(UID); // C_tid' = inc_tid(C_tid)
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID functionEntered(THREADID tid)
{
  // Helper variables
  std::string function;

  // Do not continue if the name of the entered function cannot be obtained
  if (!getCurrentFunctionName(tid, function)) return;

  // Update the window
  TLS->window->functionEntered(function);

#if VERBOSITY_LEVEL >= 10
  CONSOLE("Thread " + decstr(tid) + ": ENTER: " + function + ", vc: "
    + TLS->window->cvc + "\n");
#endif
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID functionExited(THREADID tid)
{
  // Helper variables
  std::string function;

  // Do not continue if the name of the exited function cannot be obtained
  if (!getCurrentFunctionName(tid, function)) return;

  // Update the window
  TLS->window->functionExited(function);

#if VERBOSITY_LEVEL >= 10
  CONSOLE("Thread " + decstr(tid) + ": EXIT: " + function + ", vc: "
    + TLS->window->cvc + "\n");
#endif
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Initialise locks
  PIN_MutexInit(&g_uidLock);
  PIN_RWMutexInit(&g_locksLock);

  // Preallocate the maximum number of items to prevent reallocations
  g_threads.reserve(MAX_RUNNING_THREADS);
  g_windows.reserve(MAX_TRACKED_THREADS);

  // Register callback functions called before synchronisation events
  SYNC_BeforeLockAcquire(beforeLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);
  SYNC_BeforeJoin(beforeJoin);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_AfterLockRelease(afterLockRelease);
  SYNC_AfterJoin(afterJoin);

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);
  THREAD_ThreadForked(threadForked);

  // Register callback functions called when a function is executed
  THREAD_FunctionEntered(functionEntered);
  THREAD_FunctionExited(functionExited);

  // Load the contracts to be checked
  Contract* contract = new Contract();
  contract->load("contracts");
  g_contracts.push_back(contract);

  // Dump the loaded contracts
  fs::ofstream f("contracts.dump");
  f << contract->toString();
  f.close();
}

/**
 * Cleans up the analyser.
 */
PLUGIN_FINISH_FUNCTION()
{
  // Free locks
  PIN_MutexFini(&g_uidLock);
  PIN_RWMutexFini(&g_locksLock);

  for (Contract* contract : g_contracts)
  { // Free all loaded contracts
    delete contract;
  }
}

/** End of file contract-validator.cpp **/
