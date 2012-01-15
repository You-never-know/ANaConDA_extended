/**
 * @brief Contains the entry part of the event printer module.
 *
 * A file containing the entry part of the event printer module.
 *
 * @file      event-printer.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-05
 * @date      Last Update 2012-01-05
 * @version   0.1
 */

#include <iostream>

#include "anaconda.h"

VOID beforeLockAcquire(THREADID tid, LOCK lock)
{
  std::cout << "Before lock acquired: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  std::cout << "Before lock released: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

VOID beforeSignal(THREADID tid, COND cond)
{
  std::cout << "Before signal send: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

VOID beforeWait(THREADID tid, COND cond)
{
  std::cout << "Before wait: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  std::cout << "After lock acquired: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

VOID afterLockRelease(THREADID tid, LOCK lock)
{
  std::cout << "After lock released: thread " << tid << ", lock " << lock
    << std::endl << std::flush;
}

VOID afterSignal(THREADID tid, COND cond)
{
  std::cout << "After signal send: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

VOID afterWait(THREADID tid, COND cond)
{
  std::cout << "After wait: thread " << tid << ", condition " << cond
    << std::endl << std::flush;
}

extern "C"
void init()
{
  SYNC_BeforeLockAcquire(beforeLockAcquire);
  SYNC_BeforeLockRelease(beforeLockRelease);
  SYNC_BeforeSignal(beforeSignal);
  SYNC_BeforeWait(beforeWait);

  SYNC_AfterLockAcquire(afterLockAcquire);
  SYNC_AfterLockRelease(afterLockRelease);
  SYNC_AfterSignal(afterSignal);
  SYNC_AfterWait(afterWait);
}

/** End of file event-printer.cpp **/
