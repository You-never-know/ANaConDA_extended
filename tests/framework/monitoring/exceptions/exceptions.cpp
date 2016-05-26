/**
 * @brief Tests monitoring of exceptions.
 *
 * @file      exceptions.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-01
 * @date      Last Update 2016-05-26
 * @version   0.2
 */

class TestException {};

void before_throw()
{
}

void after_throw()
{
}

void in_catch()
{
}

int main(int argc, char* argv[])
{
  try
  {
    before_throw();

    throw TestException();

    after_throw();
  }
  catch(...)
  {
    in_catch();
  }
}

/** End of file exceptions.cpp **/
