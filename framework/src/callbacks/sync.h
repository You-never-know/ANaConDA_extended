/**
 * @brief A file containing definitions of synchronisation-related callback
 *   functions.
 *
 * A file containing definitions of callback functions called when some
 *   synchronisation between threads occurs.
 *
 * @file      sync.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-02-13
 * @version   0.4.0.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__

#include <iostream>

#include "pin.H"

#include "../cbstack.h"
#include "../defs.h"
#include "../types.h"

// Definitions of functions for printing various data to a stream
API_FUNCTION std::ostream& operator<<(std::ostream& s, const LOCK& value);
API_FUNCTION std::ostream& operator<<(std::ostream& s, const COND& value);

// Definitions of functions for concatenating various data with a string
API_FUNCTION std::string operator+(const std::string& s, const LOCK& lock);
API_FUNCTION std::string operator+(const LOCK& lock, const std::string& s);
API_FUNCTION std::string operator+(const std::string& s, const COND& cond);
API_FUNCTION std::string operator+(const COND& cond, const std::string& s);

// Definitions of analysis functions (callback functions called by PIN)
VOID initSyncFunctionTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

VOID beforeLockCreate(CBSTACK_FUNC_PARAMS, VOID* funcDesc);
VOID beforeLockAcquire(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, VOID* funcDesc);
VOID beforeLockRelease(CBSTACK_FUNC_PARAMS, ADDRINT* lockAddr, VOID* funcDesc);
VOID beforeSignal(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, VOID* funcDesc);
VOID beforeWait(CBSTACK_FUNC_PARAMS, ADDRINT* condAddr, VOID* funcDesc);
VOID beforeGenericWait(CBSTACK_FUNC_PARAMS, ADDRINT* wobjAddr, VOID* funcDesc);

// Definitions of callback functions
typedef VOID (*LOCKFUNPTR)(THREADID tid, LOCK lock);
typedef VOID (*CONDFUNPTR)(THREADID tid, COND condition);

// Definitions of functions for registering callback functions
API_FUNCTION VOID SYNC_BeforeLockAcquire(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeLockRelease(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeSignal(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_BeforeWait(CONDFUNPTR callback);

API_FUNCTION VOID SYNC_AfterLockAcquire(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_AfterLockRelease(LOCKFUNPTR callback);
API_FUNCTION VOID SYNC_AfterSignal(CONDFUNPTR callback);
API_FUNCTION VOID SYNC_AfterWait(CONDFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__ */

/** End of file sync.h **/
