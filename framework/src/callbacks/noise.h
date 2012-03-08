/**
 * @brief A file containing definitions of noise injecting callback functions.
 *
 * A file containing definitions of callback functions for injecting noise.
 *
 * @file      noise.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2012-03-08
 * @version   0.1.3
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__

#include "pin.H"

// Definitions of analysis functions (callback functions called by PIN)
VOID injectSleep(THREADID tid, UINT32 frequency, UINT32 strength);
VOID injectYield(THREADID tid, UINT32 frequency, UINT32 strength);

VOID injectRsSleep(THREADID tid, UINT32 frequency, UINT32 strength);
VOID injectRsYield(THREADID tid, UINT32 frequency, UINT32 strength);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__ */

/** End of file noise.h **/
