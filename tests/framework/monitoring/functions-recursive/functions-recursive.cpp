/**
 * @brief Tests monitoring of recursive functions.
 *
 * @file      functions-recursive.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-13
 * @date      Last Update 2016-06-08
 * @version   0.2
 */

#include "../../../shared/defs.h"

void recursive_test_function(int i)
{
  FUNCTION_START

  if (i != 0)
  {
    recursive_test_function(--i);
  }

  FUNCTION_EXIT
}

int main(int argc, char* argv[])
{
  FUNCTION_START

  recursive_test_function(4);

  FUNCTION_EXIT
}

/** End of file functions-recursive.cpp **/
