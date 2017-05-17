/**
 * @brief Tests monitoring of thread creation and joining.
 *
 * @file      threads.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2017-05-17
 * @date      Last Update 2017-05-17
 * @version   0.1
 */

#include <thread>

#include "../../../shared/defs.h"

void baby_thread(int n)
{
  FUNCTION_START

  FUNCTION_EXIT
}

void child_thread(int n)
{
  FUNCTION_START

  std::thread baby (baby_thread, 2);

  baby.join();

  FUNCTION_EXIT
}

int main(int argc, char* argv[])
{
  FUNCTION_START

  std::thread child (child_thread, 1);

  child.join();

  FUNCTION_EXIT
}

/** End of file threads.cpp **/
