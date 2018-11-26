/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of TLS-related helper functions.
 *
 * A file containing definitions of functions for working with PIN's thread
 *   local storage (TLS).
 *
 * @file      tls.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-04
 * @date      Last Update 2013-06-05
 * @version   0.1.0.2
 */

#ifndef __PINTOOL_ANACONDA__PIN__TLS_H__
  #define __PINTOOL_ANACONDA__PIN__TLS_H__

#include "pin.H"

#include "../../defs.h"

API_FUNCTION TLS_KEY TLS_CreateThreadDataKey(DESTRUCTFUN dfunc);
API_FUNCTION VOID* TLS_GetThreadData(TLS_KEY key, THREADID tid);
API_FUNCTION BOOL TLS_SetThreadData(TLS_KEY key, const VOID* data, THREADID tid);

#endif /* __PINTOOL_ANACONDA__PIN__TLS_H__ */

/** End of file tls.h **/
