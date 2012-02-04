/**
 * @brief Contains definitions of TLS-related helper functions.
 *
 * A file containing definitions of functions for working with PIN's thread
 *   local storage (TLS).
 *
 * @file      tls.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-04
 * @date      Last Update 2012-02-04
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__PIN__TLS_H__
  #define __PINTOOL_ANACONDA__PIN__TLS_H__

#include "pin.H"

TLS_KEY TLS_CreateThreadDataKey(DESTRUCTFUN dfunc);
VOID* TLS_GetThreadData(TLS_KEY key, THREADID tid);
BOOL TLS_SetThreadData(TLS_KEY key, const VOID* data, THREADID tid);

#endif /* __PINTOOL_ANACONDA__PIN__TLS_H__ */

/** End of file tls.h **/
