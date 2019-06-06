/*
 * Copyright (C) 2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Defines helper functions used for debugging the framework.
 *
 * A file containing the definitions of helper functions used for debugging the
 *   framework.
 *
 * @file      debug.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-05-09
 * @date      Last Update 2019-06-04
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__DEBUG_H__
  #define __ANACONDA_FRAMEWORK__UTILS__DEBUG_H__

#include "pin.H"

#include "../config.h"

#if ANACONDA_DEBUG_MEMORY_ACCESSES == 1
  #define ASSERT_MEMORY_ACCESS(expression, message, insAddr, rtnAddr) \
    if (!(expression)) { \
      memoryAccessAssertionFailed(message, insAddr, rtnAddr); \
    } \
    assert(expression);
#else
  #define ASSERT_MEMORY_ACCESS(expression, message, insAddr, rtnAddr) \
    assert(expression);
#endif

void memoryAccessAssertionFailed(const char* message, ADDRINT insAddr,
  ADDRINT rtnAddr);

#endif /* __ANACONDA_FRAMEWORK__UTILS__DEBUG_H__ */

/** End of file debug.h **/
