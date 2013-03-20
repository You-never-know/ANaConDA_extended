/**
 * @brief A file containing definitions of noise injecting callback functions.
 *
 * A file containing definitions of callback functions for injecting noise.
 *
 * @file      noise.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2013-03-20
 * @version   0.3
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__

#include "pin.H"

#include "../settings.h"

// Definitions of wrapper functions
VOID injectSharedVariableNoise(THREADID tid, VOID* noiseDesc, ADDRINT addr,
  UINT32 size, ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers);

// Definitions of helper functions
VOID setupNoiseModule(Settings* settings);
VOID registerBuiltinNoiseFunctions();

#endif /* __PINTOOL_ANACONDA__CALLBACKS__NOISE_H__ */

/** End of file noise.h **/
