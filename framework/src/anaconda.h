/**
 * @brief Contains the ANaConDA framework's API.
 *
 * A file containing functions constituting the API of the ANaConDA framework.
 *
 * @file      anaconda.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-04
 * @date      Last Update 2014-11-13
 * @version   0.3.3
 */

#ifndef __PINTOOL_ANACONDA__ANACONDA_H__
  #define __PINTOOL_ANACONDA__ANACONDA_H__

#include <deque>
#include <vector>

#include "callbacks/exception.h"

#include "utils/pin/tls.h"

#include "defs.h"
#include "types.h"

// Definitions of memory-access-related callback functions
typedef VOID (*MEMREADAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMREADAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMREADAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMREADAVOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, BOOL isLocal);
typedef VOID (*MEMREADAVIOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, ADDRINT ins, BOOL isLocal);
typedef VOID (*MEMWRITEAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMWRITEAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMWRITEAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMWRITEAVOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, BOOL isLocal);
typedef VOID (*MEMWRITEAVIOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, ADDRINT ins, BOOL isLocal);
typedef VOID (*MEMUPDATEAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMUPDATEAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMUPDATEAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMUPDATEAVOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, BOOL isLocal);
typedef VOID (*MEMUPDATEAVIOFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, ADDRINT ins, BOOL isLocal);

// Functions for registering memory-access-related callback functions
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVIOFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVIOFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVIOFUNPTR callback);

API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVIOFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVIOFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVOFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVIOFUNPTR callback);

// Functions for retrieving information about accesses
API_FUNCTION VOID ACCESS_GetLocation(ADDRINT ins, LOCATION& location);

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

API_FUNCTION VOID THREAD_FunctionEntered(THREADFUNPTR callback);
API_FUNCTION VOID THREAD_FunctionExited(THREADFUNPTR callback);

// Functions for retrieving information about threads
API_FUNCTION VOID THREAD_GetBacktrace(THREADID tid, Backtrace& bt);
API_FUNCTION VOID THREAD_GetBacktraceSymbols(Backtrace& bt, Symbols& symbols);
API_FUNCTION VOID THREAD_GetThreadCreationLocation(THREADID tid,
  std::string& location);

// Definitions of TM-related callback functions
typedef VOID (*BEFORETXSTARTFUNPTR)(THREADID tid);
typedef VOID (*BEFORETXCOMMITFUNPTR)(THREADID tid);
typedef VOID (*BEFORETXABORTFUNPTR)(THREADID tid);
typedef VOID (*BEFORETXREADFUNPTR)(THREADID tid, ADDRINT addr);
typedef VOID (*BEFORETXWRITEFUNPTR)(THREADID tid, ADDRINT addr);

typedef VOID (*AFTERTXSTARTFUNPTR)(THREADID tid, ADDRINT* result);
typedef VOID (*AFTERTXCOMMITFUNPTR)(THREADID tid, ADDRINT* result);
typedef VOID (*AFTERTXABORTFUNPTR)(THREADID tid, ADDRINT* result);
typedef VOID (*AFTERTXREADFUNPTR)(THREADID tid, ADDRINT addr);
typedef VOID (*AFTERTXWRITEFUNPTR)(THREADID tid, ADDRINT addr);

// Functions for registering TM-related callback functions
API_FUNCTION VOID TM_BeforeTxStart(BEFORETXSTARTFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxCommit(BEFORETXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxAbort(BEFORETXABORTFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxRead(BEFORETXREADFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxWrite(BEFORETXWRITEFUNPTR callback);

API_FUNCTION VOID TM_AfterTxStart(AFTERTXSTARTFUNPTR callback);
API_FUNCTION VOID TM_AfterTxCommit(AFTERTXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_AfterTxAbort(AFTERTXABORTFUNPTR callback);
API_FUNCTION VOID TM_AfterTxRead(AFTERTXREADFUNPTR callback);
API_FUNCTION VOID TM_AfterTxWrite(AFTERTXWRITEFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__ANACONDA_H__ */

/** End of file anaconda.h **/
