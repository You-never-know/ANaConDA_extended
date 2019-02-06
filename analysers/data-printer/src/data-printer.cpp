/*
 * Copyright (C) 2016-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief An analyser printing information about the data used by functions.
 *
 * A file containing implementation of callback functions required to obtain
 *   the data used by the functions.
 *
 * @file      data-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-07-15
 * @date      Last Update 2019-02-05
 * @version   0.1.1
 */

#include <sstream>

#include "anaconda/anaconda.h"

template < typename T >
VOID printData(THREADID tid, ADDRINT* arg)
{
  // Helper variables
  std::stringstream ss;
  std::string function;

  // Get the name of the function
  THREAD_GetCurrentFunction(tid, function);

  // Print the data to a stream
  ss << (T)(*arg);

  // Print the data as a string
  CONSOLE(function + ": '" + ss.str() + "'\n");
}

/**
 * Initialises the analyser.
 */
PLUGIN_INIT_FUNCTION()
{
  // Register callback functions called when a function is executed
  THREAD_FunctionExecuted("_IO_printf", printData< const char* >, 1);
}

/** End of file data-printer.cpp **/
