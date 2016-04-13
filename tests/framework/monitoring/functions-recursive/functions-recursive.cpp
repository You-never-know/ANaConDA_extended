/**
 * @brief Tests monitoring of recursive functions.
 *
 * @file      functions-recursive.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-13
 * @date      Last Update 2016-04-13
 * @version   0.1
 */

void recursive_test_function(int i)
{
  if (i != 0)
  {
    recursive_test_function(--i);
  }
}

int main(int argc, char* argv[])
{
  recursive_test_function(4);
}

/** End of file functions-recursive.cpp **/
