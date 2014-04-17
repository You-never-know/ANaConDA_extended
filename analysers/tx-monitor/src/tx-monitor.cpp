/**
 * @brief Contains the entry part of the Tx monitor ANaConDA plugin.
 *
 * A file containing the entry part of the Tx monitor ANaConDA plugin.
 *
 * @file      tx-monitor.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-10-01
 * @date      Last Update 2014-04-17
 * @version   0.2
 */

#define MONITOR_AVERAGE_TX_TIME 0

#include "anaconda.h"

#if MONITOR_AVERAGE_TX_TIME == 1
  #include <boost/date_time/posix_time/posix_time.hpp>
#endif

#include "atomic.hpp"

#if MONITOR_AVERAGE_TX_TIME == 1
  #include "utils/scopedlock.hpp"
#endif

#if MONITOR_AVERAGE_TX_TIME == 1
// Namespace aliases
namespace pt = boost::posix_time;
#endif

namespace
{ // Static global variables (usable only within this module)
#if MONITOR_AVERAGE_TX_TIME == 1
  PIN_MUTEX g_timeLock; //!< A lock guarding access to local time.

  VOID freeTimestamp(VOID* data) { delete static_cast< pt::ptime* >(data); };

  TLS_KEY g_timestampTlsKey = TLS_CreateThreadDataKey(freeTimestamp);

  PIN_MUTEX g_txTimeLock; //!< A lock guarding access to total transaction time.

  pt::time_duration g_txTimeTotal(0, 0, 0, 0);
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
}

#if MONITOR_AVERAGE_TX_TIME == 1
// Helper macros
#define TIMESTAMP *static_cast< pt::ptime* >(TLS_GetThreadData(g_timestampTlsKey, tid))

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

VOID threadStarted(THREADID tid)
{
  TLS_SetThreadData(g_timestampTlsKey, new pt::ptime(), tid);
}
#endif

VOID beforeTxStart(THREADID tid)
{
  ATOMIC::OPS::Increment< INT64 >(&g_beforeTxStartCnt, 1);
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
#if MONITOR_AVERAGE_TX_TIME == 1
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

#if MONITOR_AVERAGE_TX_TIME == 1
  // A lock guarding access to local time
  PIN_MutexInit(&g_timeLock);

  // A lock guarding access to total transaction time
  PIN_MutexInit(&g_txTimeLock);
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
#if MONITOR_AVERAGE_TX_TIME == 1
  CONSOLE_NOPREFIX("  Average transaction execution time: "
    + decstr((g_txTimeTotal / g_afterTxCommitCnt).total_microseconds())
    + " microseconds.\n");
#endif
}

/** End of file tx-monitor.cpp **/
