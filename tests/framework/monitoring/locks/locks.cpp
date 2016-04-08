/**
 * @brief Tests monitoring of lock acquisitions and releases.
 *
 * @file      locks.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-04-08
 * @date      Last Update 2016-04-08
 * @version   0.1
 */

#include <mutex>

int main(int argc, char* argv[])
{
  std::mutex lock;

  lock.lock();
  lock.unlock();
}

/** End of file locks.cpp **/
