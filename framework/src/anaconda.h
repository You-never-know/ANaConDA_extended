/**
 * @brief Contains the ANaConDA framework's API.
 *
 * A file containing functions constituting the API of the ANaConDA framework.
 *
 * @file      anaconda.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-04
 * @date      Last Update 2013-06-17
 * @version   0.2.1
 */

#ifndef __PINTOOL_ANACONDA__ANACONDA_H__
  #define __PINTOOL_ANACONDA__ANACONDA_H__

#include "callbacks/access.h"
#include "callbacks/exception.h"
#include "callbacks/sync.h"
#include "callbacks/thread.h"

#include "utils/pin/tls.h"

#include "defs.h"

// Definitions of TM-related callback functions
typedef VOID (*TXSTARTFUNPTR)(THREADID tid);
typedef VOID (*TXCOMMITFUNPTR)(THREADID tid);
typedef VOID (*TXABORTFUNPTR)(THREADID tid);

// Functions for registering TM-related callback functions
API_FUNCTION VOID TM_BeforeTxStart(TXSTARTFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxCommit(TXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_BeforeTxAbort(TXABORTFUNPTR callback);

API_FUNCTION VOID TM_AfterTxStart(TXSTARTFUNPTR callback);
API_FUNCTION VOID TM_AfterTxCommit(TXCOMMITFUNPTR callback);
API_FUNCTION VOID TM_AfterTxAbort(TXABORTFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__ANACONDA_H__ */

/** End of file anaconda.h **/
