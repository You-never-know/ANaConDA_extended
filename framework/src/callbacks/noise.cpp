/**
 * @brief A file containing implementation of noise injecting callback
 *   functions.
 *
 * A file containing implementation of callback functions for injecting noise.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2011-11-23
 * @version   0.1
 */

#include "noise.h"

#include <stdlib.h>

/**
 * Injects a sleep noise to a program, e.g., sleeps for some amount of time.
 *
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A number of millisecond which the thread will be sleeping.
 */
VOID injectSleep(INT32 frequency, INT32 strength)
{
  if (rand() % 1000 < frequency)
  { // Inject noise (e.g. sleep for some time)
    PIN_Sleep(strength);
  }
}

/**
 * Injects a yield noise to a program, e.g., gives up the CPU.
 *
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A number of times which the thread will give up the CPU.
 */
VOID injectYield(INT32 frequency, INT32 strength)
{
  if (rand() % 1000 < frequency)
  { // Inject noise (e.g. give up the CPU)
    for (int i = 0; i < strength; i++)
    { // Give up the CPU a specific number of times
      PIN_Yield();
    }
  }
}

/** End of file noise.cpp **/
