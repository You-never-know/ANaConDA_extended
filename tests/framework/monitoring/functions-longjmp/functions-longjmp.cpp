/**
 * @brief Tests monitoring of functions when long jumps are involved.
 *
 * @file      functions-longjmp.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-21
 * @date      Last Update 2016-04-21
 * @version   0.1
 */

#include <setjmp.h>

jmp_buf g_env;

void recursive_test_function(int i)
{
  if (i != 0)
  {
    recursive_test_function(--i);
  }
  else
  {
    longjmp(g_env, 1);
  }
}

int main(int argc, char* argv[])
{
  int stop = 0;

  stop = setjmp(g_env);

  if (!stop) recursive_test_function(4);
}

/** End of file functions-longjmp.cpp **/
