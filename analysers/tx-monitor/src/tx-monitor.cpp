/*
 * Copyright (C) 2013-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains the entry part of the Tx monitor ANaConDA plugin.
 *
 * A file containing the entry part of the Tx monitor ANaConDA plugin.
 *
 * @file      tx-monitor.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-10-01
 * @date      Last Update 2019-02-06
 * @version   0.6.2
 */

#define MONITOR_AVERAGE_TX_TIME 0
#define INJECT_NOISE 1
#define PRINT_INJECTED_NOISE 0

#include "anaconda/anaconda.h"

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
  #include <boost/date_time/posix_time/posix_time.hpp>
#endif

#if INJECT_NOISE == 1
  #include <iostream>
  #include <fstream>
  #include <string>

  #include <boost/random/mersenne_twister.hpp>
  #include <boost/random/uniform_int_distribution.hpp>
#endif

#include "atomic.hpp"

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
  #include "anaconda/utils/scopedlock.hpp"
#endif

#ifdef BOOST_NO_EXCEPTIONS
// Exceptions cannot be used so we must define the throw_exception() manually
namespace boost { void throw_exception(std::exception const& e) { return; } }
#endif

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
// Namespace aliases
namespace pt = boost::posix_time;
#endif

namespace
{ // Static global variables (usable only within this module)
#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
  PIN_MUTEX g_timeLock; //!< A lock guarding access to local time.
#endif

#if MONITOR_AVERAGE_TX_TIME == 1
  VOID freeTimestamp(VOID* data) { delete static_cast< pt::ptime* >(data); };

  TLS_KEY g_timestampTlsKey = TLS_CreateThreadDataKey(freeTimestamp);

  PIN_MUTEX g_txTimeLock; //!< A lock guarding access to total transaction time.

  pt::time_duration g_txTimeTotal(0, 0, 0, 0);
#endif

#if INJECT_NOISE == 1
  VOID freeTxType(VOID* data) { delete static_cast< UINT32* >(data); };

  TLS_KEY g_txTypeTlsKey = TLS_CreateThreadDataKey(freeTxType);

  #define SHORT 0
  #define LONG 1

  typedef boost::random::mt11213b RngEngine;

  RngEngine g_rng;
  PIN_MUTEX g_rngLock;

  #define DETERMINISTIC 0
  #define PROBABILISTIC 1
  #define ROTATING 2

  const char* g_types[] = { "deterministic", "probabilistic", "rotating" };

  UINT32 g_type = 0;
  UINT32 g_txnum = 0;
  UINT32 g_ratio = 0;
  UINT32 g_shift = 0;
  UINT32 g_frequencyShort = 0;
  UINT32 g_frequencyLong = 0;
  UINT32 g_strengthShort = 0;
  UINT32 g_strengthLong = 0;
#endif

  INT64 g_beforeTxStartCnt = 0;
  INT64 g_afterTxStartCnt = 0;
  INT64 g_beforeTxCommitCnt = 0;
  INT64 g_afterTxCommitCnt = 0;
  INT64 g_afterTxCommitFailedCnt = 0;
  INT64 g_beforeTxAbortCnt = 0;
  INT64 g_afterTxAbortCnt = 0;
  INT64 g_beforeTxReadCnt = 0;
  INT64 g_afterTxReadCnt = 0;
  INT64 g_beforeTxWriteCnt = 0;
  INT64 g_afterTxWriteCnt = 0;

  INT64 g_starts[8];
  INT64 g_aborts[8];
}

#if MONITOR_AVERAGE_TX_TIME == 1
// Helper macros
#define TIMESTAMP *static_cast< pt::ptime* >(TLS_GetThreadData(g_timestampTlsKey, tid))
#endif
#if INJECT_NOISE == 1
// Helper macros
#define TX_TYPE *static_cast< UINT32* >(TLS_GetThreadData(g_txTypeTlsKey, tid))
#endif

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
/**
 * Gets the current time.
 *
 * @return The current time.
 */
inline
pt::ptime getTime()
{
  // Boost implementation of local_time() might call functions from the C or C++
  // libraries, but C and C++ libraries linked into pintools are not thread-safe
  ScopedLock lock(g_timeLock);

  return pt::microsec_clock::local_time(); // Now we can safely access the time
}
#endif

#if INJECT_NOISE == 1
/**
 * Generates a random frequency, i.e., an integer number from 0 to 999.
 *
 * @return An integer number from 0 to 999.
 */
inline
UINT32 randomFrequency()
{
  // Restrict the generated integer to the <0, 999> interval
  boost::random::uniform_int_distribution<> dist(0, 999);

  // Random number generation is not thread-safe, must be done exclusively
  ScopedLock lock(g_rngLock);

  // Generate a new random integer from the <0, 999> interval
  return dist(g_rng);
}

/**
 * Injects a noise to a program.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A concrete strength of the noise.
 */
inline
VOID injectNoise(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // We are under the frequency threshold, insert the noise
    pt::ptime end = getTime() + pt::microseconds(strength);

#if PRINT_INJECTED_NOISE == 1
    pt::ptime now; // Helper variables

    while ((now = getTime()) < end)
#else
    while (getTime() < end)
#endif
    { // Strength determines how many loop iterations we should perform
#if PRINT_INJECTED_NOISE == 1
      CONSOLE("Thread " + decstr(tid) + ": looping ("
        + decstr((end - now).total_microseconds())
        + " microseconds remaining).\n");
#endif

      frequency++; // No need to define new local variable, reuse frequency
    }
  }
}
#endif

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
VOID threadStarted(THREADID tid)
{
#if MONITOR_AVERAGE_TX_TIME == 1
  TLS_SetThreadData(g_timestampTlsKey, new pt::ptime(), tid);
#endif
#if INJECT_NOISE == 1
  TLS_SetThreadData(g_txTypeTlsKey, new UINT32, tid);
#endif
}
#endif

VOID beforeTxStart(THREADID tid)
{
#if INJECT_NOISE == 1
  if (g_type == ROTATING)
  {
    if (ATOMIC::OPS::Increment< INT64 >(&g_beforeTxStartCnt, 1) % g_txnum == 0)
    {
      g_shift = (g_shift + g_ratio) % 8;
    }
  }
  else
  {
    ATOMIC::OPS::Increment< INT64 >(&g_beforeTxStartCnt, 1);
  }

  if (g_type == DETERMINISTIC)
  { // Ratio determines the number of threads generating short transactions
    if (tid < g_ratio)
    { // A thread generating the short transactions, inject noise
      injectNoise(tid, g_frequencyShort, g_strengthShort);
    }
  }
  else if (g_type == ROTATING)
  { // Ratio determines the number of threads generating short transactions
    if (((tid + g_shift) % 8) < g_ratio)
    { // A thread generating the short transactions, inject noise
      injectNoise(tid, g_frequencyShort, g_strengthShort);
    }
  }
  else if (g_type == PROBABILISTIC)
  { // Ration determines the probability of a transaction to be a short one
    if (randomFrequency() < g_ratio)
    { // This transaction will be a short one, inject noise
      TX_TYPE = SHORT;
      injectNoise(tid, g_frequencyShort, g_strengthShort);
    }
    else
    { // This transaction will be a long one, do NOT inject noise
      TX_TYPE = LONG;
    }
  }
#else
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxStartCnt, 1);
#endif

  g_starts[tid]++;
//  CONSOLE("Before thread " + decstr(tid) + " starts a transaction\n");
}

VOID afterTxStart(THREADID tid, ADDRINT* result)
{
  ATOMIC::OPS::Increment< INT64 >(&g_afterTxStartCnt, 1);

#if MONITOR_AVERAGE_TX_TIME == 1
  TIMESTAMP = getTime();
#endif
//  CONSOLE("After thread " + decstr(tid) + " starts a transaction\n");
}

VOID beforeTxCommit(THREADID tid)
{
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxCommitCnt, 1);

#if INJECT_NOISE == 1
  if (g_type == DETERMINISTIC)
  { // Ratio determines the number of threads generating short transactions
    if (tid >= g_ratio)
    { // A thread generating the long transactions, inject noise
      injectNoise(tid, g_frequencyLong, g_strengthLong);
    }
  }
  else if (g_type == ROTATING)
  { // Ratio determines the number of threads generating short transactions
    if (((tid + g_shift) % 8) >= g_ratio)
    { // A thread generating the long transactions, inject noise
      injectNoise(tid, g_frequencyLong, g_strengthLong);
    }
  }
  else if (g_type == PROBABILISTIC)
  { // Ration determines the probability of a transaction to be a short one
    if (TX_TYPE == LONG)
    { // This transaction is a long transaction, inject noise
      injectNoise(tid, g_frequencyLong, g_strengthLong);
    }
  }
#endif
//  CONSOLE("Before thread " + decstr(tid) + " commits a transaction\n");
}

VOID afterTxCommit(THREADID tid, ADDRINT* result)
{
  if (result != NULL && *result == 1)
  {
    ATOMIC::OPS::Increment< INT64 >(&g_afterTxCommitCnt, 1);

#if MONITOR_AVERAGE_TX_TIME == 1
    pt::time_duration txTime = getTime() - TIMESTAMP;

    ScopedLock lock(g_txTimeLock);

    g_txTimeTotal += txTime;
#endif
//    CONSOLE("Thread " + decstr(tid) + ": transaction executed in "
//      + decstr(txtime.total_microseconds()) + " microseconds.\n");
  }
  else
  {
    ATOMIC::OPS::Increment< INT64 >(&g_afterTxCommitFailedCnt, 1);
  }
//  CONSOLE("After thread " + decstr(tid) + " commits a transaction\n");
}

VOID beforeTxAbort(THREADID tid)
{
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxAbortCnt, 1);

  g_aborts[tid]++;
//  CONSOLE("Before thread " + decstr(tid) + " aborts a transaction\n");
}

VOID afterTxAbort(THREADID tid, ADDRINT* result)
{
  ATOMIC::OPS::Increment< INT64 >(&g_afterTxAbortCnt, 1);
//  CONSOLE("After thread " + decstr(tid) + " aborts a transaction\n");
}

VOID beforeTxRead(THREADID tid, ADDRINT addr)
{
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxReadCnt, 1);
//  CONSOLE("Before thread " + decstr(tid)
//    + " read from memory address " + hexstr(addr) + " within a transaction.\n");
}

VOID afterTxRead(THREADID tid, ADDRINT addr)
{
  ATOMIC::OPS::Increment< INT64 >(&g_afterTxReadCnt, 1);
//  CONSOLE("After thread " + decstr(tid)
//    + " read from memory address " + hexstr(addr) + " within a transaction.\n");
}

VOID beforeTxWrite(THREADID tid, ADDRINT addr)
{
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxWriteCnt, 1);
//  CONSOLE("Before thread " + decstr(tid)
//    + " written to memory address " + hexstr(addr) + " within a transaction.\n");
}

VOID afterTxWrite(THREADID tid, ADDRINT addr)
{
  ATOMIC::OPS::Increment< INT64 >(&g_afterTxWriteCnt, 1);
//  CONSOLE("After thread " + decstr(tid)
//    + " written to memory address " + hexstr(addr) + " within a transaction.\n");
}

/**
 * Initialises the Tx monitor plugin.
 */
PLUGIN_INIT_FUNCTION()
{
#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
  THREAD_ThreadStarted(threadStarted);
#endif

  TM_BeforeTxStart(beforeTxStart);
  TM_BeforeTxCommit(beforeTxCommit);
  TM_BeforeTxAbort(beforeTxAbort);
  TM_BeforeTxRead(beforeTxRead);
  TM_BeforeTxWrite(beforeTxWrite);

  TM_AfterTxStart(afterTxStart);
  TM_AfterTxCommit(afterTxCommit);
  TM_AfterTxAbort(afterTxAbort);
  TM_AfterTxRead(afterTxRead);
  TM_AfterTxWrite(afterTxWrite);

#if MONITOR_AVERAGE_TX_TIME == 1 || INJECT_NOISE == 1
  // A lock guarding access to local time
  PIN_MutexInit(&g_timeLock);
#endif

#if MONITOR_AVERAGE_TX_TIME == 1
  // A lock guarding access to total transaction time
  PIN_MutexInit(&g_txTimeLock);
#endif

#if INJECT_NOISE == 1
  // Initialise a lock guarding access to the random number generator
  PIN_MutexInit(&g_rngLock);

  // Initialise the random number generator
  g_rng.seed(static_cast< RngEngine::result_type >(
    getTime().time_of_day().fractional_seconds()));

  std::string line;
  std::ifstream nconfig("conf/noise.conf");
  getline(nconfig, line);
  if (line == "deterministic") g_type = DETERMINISTIC;
  else if (line == "probabilistic") g_type = PROBABILISTIC;
  else if (line == "rotating")
  {
    g_type = ROTATING;
    getline(nconfig, line);
    std::istringstream(line) >> g_txnum;
  }
  getline(nconfig, line);
  std::istringstream(line) >> g_ratio;
  getline(nconfig, line);
  std::istringstream(line) >> g_frequencyShort;
  getline(nconfig, line);
  std::istringstream(line) >> g_strengthShort;
  getline(nconfig, line);
  std::istringstream(line) >> g_frequencyLong;
  getline(nconfig, line);
  std::istringstream(line) >> g_strengthLong;
  nconfig.close();
#endif
}

PLUGIN_FINISH_FUNCTION()
{
  CONSOLE_NOPREFIX("Tx Monitor finished:\n");
  CONSOLE_NOPREFIX("  Transactions started: " + decstr(g_beforeTxStartCnt)
    + " (" + decstr(g_afterTxStartCnt) + " succeeded)\n");
  CONSOLE_NOPREFIX("  Transactions commited: " + decstr(g_beforeTxCommitCnt)
    + " (" + decstr(g_afterTxCommitCnt) + " succeeded, "
    + decstr(g_afterTxCommitFailedCnt) + " failed)\n");
  CONSOLE_NOPREFIX("  Transactions aborted: " + decstr(g_beforeTxAbortCnt)
    + " (" + decstr(g_afterTxAbortCnt) + " succeeded)\n");
  CONSOLE_NOPREFIX("  Transactional reads: " + decstr(g_beforeTxReadCnt)
    + " (" + decstr(g_afterTxReadCnt) + " succeeded)\n");
  CONSOLE_NOPREFIX("  Transactional writes: " + decstr(g_beforeTxWriteCnt)
    + " (" + decstr(g_afterTxWriteCnt) + " succeeded)\n");
  std::string starts;
  std::string aborts;
  for (int i = 0; i < 8; i++)
  {
    starts += "," + decstr(g_starts[i]);
    aborts += "," + decstr(g_aborts[i]);
  }
  starts[0] = ' ';
  aborts[0] = ' ';
  CONSOLE_NOPREFIX("  Transactions started per-thread:" + starts + "\n");
  CONSOLE_NOPREFIX("  Transactions aborted per-thread:" + aborts + "\n");
#if MONITOR_AVERAGE_TX_TIME == 1
  CONSOLE_NOPREFIX("  Average transaction execution time: "
    + decstr((g_txTimeTotal / g_afterTxCommitCnt).total_microseconds())
    + " microseconds.\n");
#endif
#if INJECT_NOISE == 1
  CONSOLE_NOPREFIX("  Type of short-transaction threads: "
    + std::string(g_types[g_type]) + ((g_type == ROTATING) ? " (rotate after "
    + decstr(g_txnum) + " transactions)" : "") + " \n");
  CONSOLE_NOPREFIX("  Ratio of short-transaction threads: " + decstr(g_ratio)
    + " \n");
  CONSOLE_NOPREFIX("  Short-transaction noise: frequency "
    + decstr(g_frequencyShort) + ", strength " + decstr(g_strengthShort)
    + " \n");
  CONSOLE_NOPREFIX("  Long-transaction noise: frequency "
    + decstr(g_frequencyLong) + ", strength " + decstr(g_strengthLong)
    + " \n");
#endif
}

/** End of file tx-monitor.cpp **/
