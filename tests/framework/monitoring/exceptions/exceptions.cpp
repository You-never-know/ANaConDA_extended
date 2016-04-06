/**
 * @brief Tests monitoring of exceptions.
 *
 * @file      exceptions.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-01
 * @date      Last Update 2016-04-01
 * @version   0.1
 */

class TestException {};

int main(int argc, char* argv[])
{
  try
  {
    throw TestException();
  }
  catch(...) {}
}

/** End of file exceptions.cpp **/
