/*
 * Copyright (C) 2011-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
