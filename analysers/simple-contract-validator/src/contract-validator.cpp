/*
 * Copyright (C) 2014-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @date      Created 2014-11-27
 * @date      Last Update 2019-02-06
 * @version   0.9.3
 */

#include "anaconda/anaconda.h"

#include <list>
#include <map>
#include <regex>
#include <set>
#include <vector>

#include <boost/foreach.hpp>

#include "../../simple-contract-validator/src/contract.h"
#include "../../simple-contract-validator/src/vc.hpp"

namespace
{ // Internal type definitions and variables (usable only within this module)
  typedef std::list< FARunner* > CheckedContracts;
  typedef std::set< LOCK > LockSet;

  /**
   * @brief A structure holding private data of a thread.
   */
  typedef struct ThreadData_s
  {
    THREADID tid; //!< A thread owning the data.
    CheckedContracts cc; //!< A list of currently checked contracts.
    CheckedContracts ccv; //!< A list of currently checked contract violations.
    LockSet lockset; //!< A set of locks held by a thread.
    VectorClock cvc; //!< The current vector clock of a thread.

    /**
     * Constructs a ThreadData_s object.
     *
     * @param t A thread owning the data.
     */
    ThreadData_s(THREADID t) : tid(t), cc(), lockset(), cvc()
    {
      cvc.init(tid);
    }
  } ThreadData;

  TLS_KEY g_tlsKey = TLS_CreateThreadDataKey(
    [] (VOID* data) { delete static_cast< ThreadData* >(data); }
  );

  std::list< Contract* > g_contracts; //!< A list of loaded contracts.

  typedef std::map< LOCK, VectorClock > LockVectorClocks;
  LockVectorClocks g_locks; //!< Vector clocks for locks.
  PIN_RWMUTEX g_locksLock; //!< A lock guarding access to @c g_locks map.

  /**
   * @brief A structure holding information about vector clocks for sequences.
   */
  typedef struct StateWithVectorClock_s
  {
    VectorClock vc; //!< A vector clock.
    FA::State* state; //!< A state to which is the vector clock assigned.
  } StateWithVectorClock;

  typedef std::vector< StateWithVectorClock > SequenceVectorClocks;
  SequenceVectorClocks g_starts; //!< Vector clocks for sequence starts.
  SequenceVectorClocks g_ends; //!< Vector clocks for sequence ends.
  PIN_RWMUTEX g_startsLock; //!< A lock guarding access to @c g_starts vector.
  PIN_RWMUTEX g_endsLock; //!< A lock guarding access to @c g_ends vector.
}

// Helper macros
#define TLS static_cast< ThreadData* >(TLS_GetThreadData(g_tlsKey, tid))

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  TLS->lockset.insert(lock);
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  TLS->lockset.erase(lock);

  for (CheckedContracts::iterator it = TLS->cc.begin(); it != TLS->cc.end();
    ++it)
  {
    (*it)->lockset.erase(lock);
  }

  PIN_RWMutexWriteLock(&g_locksLock);

  // Update the last lock release, L_m' = C_t
  g_locks[lock] = TLS->cvc;

  PIN_RWMutexUnlock(&g_locksLock);

  TLS->cvc.increment(tid); // Move to the next epoch, C_t' = inc_t(C_t)
}

/**
 * TODO
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  PIN_RWMutexReadLock(&g_locksLock);

  try
  {
    TLS->cvc.join(g_locks.at(lock));
  }
  catch (std::out_of_range& e) {}

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
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  TLS_SetThreadData(g_tlsKey, new ThreadData(tid), tid);
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID threadFinished(THREADID tid)
{
  //
}

VOID contractSequenceStarted(THREADID tid, FARunner* contract)
{
  PIN_RWMutexWriteLock(&g_startsLock);

  // Update the last sequence start, VC_start(as)(t) = C_t(t)
  contract->state()->vc.update(tid, TLS->cvc.vc[tid]);

  contract->state()->violations.erase(tid);

  PIN_RWMutexUnlock(&g_startsLock);
}

VOID contractSequenceEnded(THREADID tid, FARunner* contract)
{
  PIN_RWMutexWriteLock(&g_startsLock);

  // Update the last sequence start, VC_start(as)(t) = C_t(t)
  contract->state()->vc.update(tid, TLS->cvc.vc[tid]);

  if (contract->state()->start->violations.count(tid) > 0)
  { // We already violated this contract before, but we did not know if it is
    // a valid contract, now that the contract has been validated, it is error
    CONSOLE("Detected contract violation in thread " + decstr(tid)
      + "! Sequence violated:" + contract->state()->sequence
      + " [HB method].\n");

    contract->state()->start->violations.erase(tid);
  }

  PIN_RWMutexUnlock(&g_startsLock);
}

VOID contractViolationStarted(THREADID tid, FARunner* violation)
{
  PIN_RWMutexWriteLock(&g_startsLock);

  // Update the last violation start, VC_start(vs)(t) = C_t(t)
  violation->state()->vc.update(tid, TLS->cvc.vc[tid]);

  PIN_RWMutexUnlock(&g_startsLock);
}

VOID contractViolationEnded(THREADID tid, FARunner* violation)
{
  PIN_RWMutexWriteLock(&g_startsLock);

  // Update the last violation start, VC_start(vs)(t) = C_t(t)
  violation->state()->vc.update(tid, TLS->cvc.vc[tid]);

  PIN_RWMutexUnlock(&g_startsLock);
}

VOID contractViolationStartedAndEnded(THREADID tid, FARunner* violation)
{
  PIN_RWMutexWriteLock(&g_startsLock);

  // Update the last violation start, VC_start(vs)(t) = C_t(t)
  violation->state()->vc.update(tid, TLS->cvc.vc[tid]);

  BOOST_FOREACH(FA::State* contractState, violation->state()->conflicts)
  { //
    Threads violations;

    // If VC_end(as) </= VC_vs, then this violation might interleave a contract
    contractState->vc.notHB(TLS->cvc, violations);

    if (!violations.empty())
    { // At least one thread has not synchronised end(as) with vs
      CONSOLE("Detected contract violation in thread "
        + decstr(*violations.begin()) + "! Sequence violated:"
        + contractState->sequence + " [HB method].\n");
    }

    Threads possibleViolations;

    // If we saw VC_start(as) before, then we may cause a violation if the
    // contract is accepted
    contractState->start->vc.seen(possibleViolations);

    BOOST_FOREACH(THREADID t, possibleViolations)
    { // Flag the contract as having a possible violation in all threads, where
      // the contract has started, but was not accepted yet
      if (violations.count(t) == 0 && tid != t)
        contractState->start->violations.insert(t);
    }
  }

  PIN_RWMutexUnlock(&g_startsLock);
}

/**
 * TODO
 *
 * @param tid A number identifying the thread.
 */
VOID functionEntered(THREADID tid)
{
  // Helper variables
  std::regex re(".*!([a-zA-Z0-9_:]+)");
  std::smatch mo;
  std::string function;

  // Get a full signature of the currently executed function
  THREAD_GetCurrentFunction(tid, function);

  // Extract the function name from the signature
  regex_match(function, mo, re);

  // Ignore functions whose names cannot be obtained
  if ((function = mo[1].str()).empty()) return;

  // Helper variables
  FARunner* cc;
  FARunner* ccv;

  // First try to detect violations in the currently checked contracts
  CheckedContracts::iterator it = TLS->cc.begin();

  while (it != TLS->cc.end())
  { // Try to advance to the next function of this contract's method sequence
    if ((*it)->advance(function))
    { // We advanced to the next state (or encountered method not in alphabet)
      if ((*it)->accepted())
      { // We got to the end of some method sequence of this contract
        if ((*it)->lockset.empty())
        {
          CONSOLE("Detected contract violation in thread " + decstr(tid)
            + "! Sequence violated:" + (*it)->sequence()
            + " [Lockset method].\n");
        }

        contractSequenceEnded(tid, *it);

        TLS->cc.erase(it++);
      }
      else
      { // This contract is not accepted yet, move to the next checked contract
        ++it;
      }
    }
    else
    { // We failed to advance to the next state through a method from alphabet,
      TLS->cc.erase(it++); // so this cannot be a valid contract, stop checking
    }
  }

  it = TLS->ccv.begin();

  while (it != TLS->ccv.end())
  { // Try to advance to the next function of this contract's violation sequence
    if ((*it)->advance(function))
    { // We advanced to the next state (or encountered method not in alphabet)
      if ((*it)->accepted())
      { // We got to the end of some method sequence of this contract

        contractViolationEnded(tid, *it);

        TLS->ccv.erase(it++);
      }
      else
      { // This contract is not accepted yet, move to the next checked contract
        ++it;
      }
    }
    else
    { // We failed to advance to the next state through a method from alphabet,
      TLS->ccv.erase(it++); // so this cannot be a valid contract, stop checking
    }
  }

  // Then add new contracts to be checked if necessary
  BOOST_FOREACH(Contract* contract, g_contracts)
  { // Check if the function is not the first function of some method sequence
    if ((cc = contract->startsWith(function)) != NULL)
    { // Add this contract to the list of checked contracts
      cc->advance(function);

      // This locks were held when we started checking the contract
      cc->lockset.insert(TLS->lockset.begin(), TLS->lockset.end());

      contractSequenceStarted(tid, cc);

      TLS->cc.push_back(cc);
    }

    if ((ccv = contract->violationStartsWith(function)) != NULL)
    { // Add this contract violation to the list of checked contract violations
      ccv->advance(function);

      if (ccv->accepted())
      { // Violation sequence consisting of a single method
        contractViolationStartedAndEnded(tid, ccv);
      }
      else
      { // Violation sequence consisting of more that one method
        contractViolationStarted(tid, ccv);

        TLS->ccv.push_back(ccv);
      }
    }
  }
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Initialise locks
  PIN_RWMutexInit(&g_locksLock);
  PIN_RWMutexInit(&g_startsLock);
  PIN_RWMutexInit(&g_endsLock);

  // Register callback functions called before synchronisation events
  SYNC_BeforeLockAcquire(beforeLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_AfterLockRelease(afterLockRelease);

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);
  THREAD_ThreadFinished(threadFinished);

  // Register callback functions called when a function is executed
  THREAD_FunctionEntered(functionEntered);

  // Load the contracts
  Contract* contract = new Contract();
  contract->load("contracts");
  g_contracts.push_back(contract);
}

/**
 * Cleans up the analyser.
 */
PLUGIN_FINISH_FUNCTION()
{
  // Free locks
  PIN_RWMutexFini(&g_locksLock);
  PIN_RWMutexFini(&g_startsLock);
  PIN_RWMutexFini(&g_endsLock);
}

/** End of file contract-validator.cpp **/
