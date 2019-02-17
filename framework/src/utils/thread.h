/*
 * Copyright (C) 2013-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of thread-related utilities.
 *
 * A file containing definitions of various thread-related utility functions.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-04
 * @date      Last Update 2016-05-09
 * @version   0.3.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__THREAD_H__
  #define __PINTOOL_ANACONDA__UTILS__THREAD_H__

#include "pin.H"

#include "../types.h"

// Definitions of callback functions
typedef VOID (*THREADINITFUNPTR)(THREADID tid, VOID* data);

// Definitions of utility functions for registering thread init functions
VOID addThreadInitFunction(THREADINITFUNPTR callback, VOID* data);

// Definitions of utility functions for accessing thread information
THREADID getThreadId(THREAD thread);

// Definitions of utility functions for accessing backtrace information
index_t getLastBacktraceLocationIndex(THREADID tid);
std::string getLastBacktraceLocation(THREADID tid);
size_t getBacktraceSize(THREADID tid);

#endif /* __PINTOOL_ANACONDA__UTILS__THREAD_H__ */

/** End of file thread.h **/
