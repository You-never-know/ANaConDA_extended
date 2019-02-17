/*
 * Copyright (C) 2016-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of functions for instrumenting unwind hooks.
 *
 * A file containing definitions of functions for instrumenting unwind hooks.
 *
 * @file      unwind.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-07
 * @date      Last Update 2016-06-07
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__
  #define __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__

#include "pin.H"

// Type definitions
typedef VOID (*UNWINDFUNPTR)(THREADID tid, ADDRINT sp);

// Functions for instrumenting unwind hooks
VOID instrumentUnwindFunction(RTN rtn, UNWINDFUNPTR callback);

#endif /* __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__ */

/** End of file unwind.h **/
