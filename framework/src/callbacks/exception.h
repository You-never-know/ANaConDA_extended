/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief A file containing definitions of exception-related callback
 *   functions.
 *
 * A file containing definitions of callback functions called when an
 *   exception is thrown or caught.
 *
 * @file      exception.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-29
 * @date      Last Update 2012-06-01
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__EXCEPTION_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__EXCEPTION_H__

#include "pin.H"

#include "../defs.h"

/**
 * @brief A structure representing an exception.
 */
typedef struct Exception_s
{
  /**
   * @brief A name of the class or structure representing the exception.
   */
  std::string name;

  /**
   * Constructs an Exception_s object.
   */
  Exception_s() : name() {}

  /**
   * Constructs an Exception_s object.
   *
   * @param n A name of the class or structure representing the exception.
   */
  Exception_s(const std::string& n) : name(n) {}
} EXCEPTION;

// Definitions of analysis functions (callback functions called by PIN)
VOID beforeThrow(THREADID tid, ADDRINT thrown_exception, ADDRINT tinfo);
VOID afterBeginCatch(THREADID tid, ADDRINT exceptionObject, CONTEXT* registers);

// Definitions of callback functions
typedef VOID (*EXCEPTIONFUNPTR)(THREADID tid, const EXCEPTION& exception);

// Definitions of functions for registering callback functions
API_FUNCTION VOID EXCEPTION_ExceptionThrown(EXCEPTIONFUNPTR callback);
API_FUNCTION VOID EXCEPTION_ExceptionCaught(EXCEPTIONFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__EXCEPTION_H__ */

/** End of file exception.h **/
