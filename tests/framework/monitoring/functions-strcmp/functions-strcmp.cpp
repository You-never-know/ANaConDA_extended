/**
 * @brief Tests monitoring of the strcmp function.
 *
 * The strcmp function may jump at its beginning during its execution which is
 *   problematic as the ANaConDA framework determines if a function started by
 *   checking if the program is about to execute the first instruction of the
 *   function.
 *
 * @file      functions-strcmp.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-13
 * @date      Last Update 2016-04-19
 * @version   0.1
 */

#include <stdio.h>
#include <string.h>

const char* first = "test string";
const char* second = "test string";

void before_strcmp()
{
}

void after_strcmp()
{
}

int main(int argc, char* argv[])
{
  before_strcmp();

  int result = strcmp(first, second); // Optimiser cannot omit this

  after_strcmp();

  return result;
}

/** End of file functions-strcmp.cpp **/
