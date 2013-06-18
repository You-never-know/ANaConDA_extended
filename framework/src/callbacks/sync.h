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
 * @date      Last Update 2013-06-18
 * @version   0.9
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__

#include <iostream>

#include "pin.H"

#include "../cbstack.h"
#include "../defs.h"
#include "../settings.h"
#include "../types.h"

// Definitions of helper functions
VOID setupSyncModule(Settings* settings);

// Definitions of analysis functions (callback functions called by PIN)
VOID beforeLockCreate(CBSTACK_FUNC_PARAMS, HookInfo* hi);
template< ConcurrentCoverage CC >
VOID beforeLockAcquire(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
template< ConcurrentCoverage CC >
VOID beforeLockRelease(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
VOID beforeSignal(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
VOID beforeWait(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);
template< ConcurrentCoverage CC >
VOID beforeGenericWait(CBSTACK_FUNC_PARAMS, ADDRINT* arg, HookInfo* hi);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__SYNC_H__ */

/** End of file sync.h **/
