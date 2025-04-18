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
 * @brief A file containing implementation of exception-related callback
 *   functions.
 *
 * A file containing implementation of callback functions called when an
 *   exception is thrown or caught.
 *
 * @file      exception.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-29
 * @date      Last Update 2016-06-10
 * @version   0.1.3
 */

#include "exception.h"

#include <assert.h>

#include <typeinfo>

#include "thread.h"

#ifdef TARGET_LINUX
// Returns the type of the currently handled exception (catch does not know it)
extern "C" std::type_info* __cxa_current_exception_type();
#endif

namespace
{ // Static global variables (usable only within this module)
  typedef std::vector< EXCEPTIONFUNPTR > ExceptionFunPtrVector;

  ExceptionFunPtrVector g_exceptionThrownVector;
  ExceptionFunPtrVector g_exceptionCaughtVector;
}

/**
 * Calls all callback functions registered by a user to be called when an
 *   exception is thrown.
 *
 * @param tid A number identifying the thread which has thrown the exception.
 * @param thrown_exception An object representing the exception.
 * @param tinfo An object containing information about the type of the object
 *   representing the exception.
 */
VOID beforeThrow(THREADID tid, ADDRINT thrown_exception, ADDRINT tinfo)
{
  // Get the type of the object representing the exception to be thrown
  EXCEPTION e(PIN_UndecorateSymbolName(reinterpret_cast< std::type_info* >
    (tinfo)->name(), UNDECORATION_COMPLETE));

  for (ExceptionFunPtrVector::iterator it = g_exceptionThrownVector.begin();
    it != g_exceptionThrownVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, e);
  }
}

/**
 * Calls all callback functions registered by a user to be called when an
 *   exception is caught.
 *
 * @param tid A number identifying the thread which has caught the exception.
 * @param exceptionObject An object representing the exception.
 * @param registers A structure containing register values.
 */
VOID afterBeginCatch(THREADID tid, ADDRINT exceptionObject, CONTEXT* registers)
{
#ifdef TARGET_LINUX
  // Catch do not take the type info of the exception object as a parameter
  void *tinfo = NULL;

  // When calling an application function, PIN does not actually execute a CALL
  // instruction, it just updates the stack to a state in which the stack would
  // be if the CALL was issued and then resumes the execution, however, we need
  // to notify the analysers that this CALL actually occurred in the program
  beforeFunctionCalled(tid, PIN_GetContextReg(registers, REG_STACK_PTR)
    - sizeof(ADDRINT), 0);

  // But we can get the type info of the currently handled exception object
  PIN_CallApplicationFunction(registers, tid, CALLINGSTD_DEFAULT, (AFUNPTR)
    __cxa_current_exception_type, NULL, PIN_PARG(void*), &tinfo, PIN_PARG_END()
  );

  // We are in a catch block, so there must be an exception being handled
  assert(tinfo != NULL);

  // Get the type of the object representing the exception caught
  EXCEPTION e(PIN_UndecorateSymbolName(reinterpret_cast< std::type_info* >
    (tinfo)->name(), UNDECORATION_COMPLETE));

  for (ExceptionFunPtrVector::iterator it = g_exceptionCaughtVector.begin();
    it != g_exceptionCaughtVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, e);
  }
#endif
}

/**
 * Registers a callback function which will be called when an exception is
 *   thrown.
 *
 * @param callback A callback function which should be called when an exception
 *   is thrown.
 */
VOID EXCEPTION_ExceptionThrown(EXCEPTIONFUNPTR callback)
{
  g_exceptionThrownVector.push_back(callback);
}

/**
 * Registers a callback function which will be called when an exception is
 *   caught.
 *
 * @param callback A callback function which should be called when an exception
 *   is caught.
 */
VOID EXCEPTION_ExceptionCaught(EXCEPTIONFUNPTR callback)
{
  g_exceptionCaughtVector.push_back(callback);
}

/** End of file exception.cpp **/
