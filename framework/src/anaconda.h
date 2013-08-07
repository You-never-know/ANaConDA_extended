/**
 * @brief Contains the ANaConDA framework's API.
 *
 * A file containing functions constituting the API of the ANaConDA framework.
 *
 * @file      anaconda.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-04
 * @date      Last Update 2013-08-07
 * @version   0.2.6
 */

#ifndef __PINTOOL_ANACONDA__ANACONDA_H__
  #define __PINTOOL_ANACONDA__ANACONDA_H__

#include "callbacks/access.h"
#include "callbacks/exception.h"

#include "utils/pin/tls.h"

#include "defs.h"

// Definitions of synchronisation-related callback functions
typedef VOID (*LOCKFUNPTR)(THREADID tid, LOCK lock);
typedef VOID (*CONDFUNPTR)(THREADID tid, COND condition);
typedef VOID (*JOINFUNPTR)(THREADID tid, THREADID jtid);

// Functions for registering synchronisation-related callback functions
API_FUNCTION VOID SYNC_BeforeLockAcquire(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeLockRelease(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeSignal(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeWait(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeJoin(JOINFUNPTR callback);

API_FUNCTION VOID SYNC_AfterLockAcquire(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_AfterLockRelease(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_AfterSignal(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_AfterWait(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_AfterJoin(JOINFUNPTR callback);

// Definitions of thread-related special data types
typedef std::deque< ADDRINT > Backtrace;
typedef std::vector< std::string > Symbols;

// Definitions of thread-related callback functions
typedef VOID (*THREADFUNPTR)(THREADID tid);

// Functions for registering thread-related callback functions
API_FUNCTION VOID THREAD_ThreadStarted(THREADFUNPTR callback);
API_FUNCTION VOID THREAD_ThreadFinished(THREADFUNPTR callback);

// Functions for retrieving information about threads
API_FUNCTION VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt);
API_FUNCTION VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols);
API_FUNCTION VOID THREAD_GetThreadCreationLocation(THREADID tid,
  std::string& location);

// Definitions of TM-related callback functions
typedef VOID (*TXSTARTFUNPTR)(THREADID tid);
typedef VOID (*TXCOMMITFUNPTR)(THREADID tid);
typedef VOID (*TXABORTFUNPTR)(THREADID tid);
typedef VOID (*TXREADFUNPTR)(THREADID tid, ADDRINT addr);
typedef VOID (*TXWRITEFUNPTR)(THREADID tid, ADDRINT addr);

// Functions for registering TM-related callback functions
API_FUNCTION VOID TM_BeforeTxStart(TXSTARTFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxCommit(TXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxAbort(TXABORTFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxRead(TXREADFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxWrite(TXWRITEFUNPTR callback);

API_FUNCTION VOID TM_AfterTxStart(TXSTARTFUNPTR callback);
API_FUNCTION VOID TM_AfterTxCommit(TXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_AfterTxAbort(TXABORTFUNPTR callback);
API_FUNCTION VOID TM_AfterTxRead(TXREADFUNPTR callback);
API_FUNCTION VOID TM_AfterTxWrite(TXWRITEFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__ANACONDA_H__ */

/** End of file anaconda.h **/
