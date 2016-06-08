/**
 * @brief Tests monitoring of functions when long jumps are involved.
 *
 * @file      functions-longjmp.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-21
 * @date      Last Update 2016-06-08
 * @version   0.2
 */

#include <setjmp.h>

#include "../../../shared/defs.h"

jmp_buf g_env;

void recursive_test_function(int i)
{
  FUNCTION_START

  if (i != 0)
  {
    recursive_test_function(--i);
  }
  else
  {
    FUNCTION_EXIT

    longjmp(g_env, 1);
  }

  FUNCTION_EXIT
}

int main(int argc, char* argv[])
{
  FUNCTION_START

  int stop = 0;

  stop = setjmp(g_env);

  if (!stop) recursive_test_function(4);

  FUNCTION_EXIT
}

/** End of file functions-longjmp.cpp **/
