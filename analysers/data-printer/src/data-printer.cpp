/**
 * @brief An analyser printing information about the data used by functions.
 *
 * A file containing implementation of callback functions required to obtain
 *   the data used by the functions.
 *
 * @file      data-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-07-15
 * @date      Last Update 2016-07-15
 * @version   0.1
 */

#include <sstream>

#include "anaconda.h"

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
