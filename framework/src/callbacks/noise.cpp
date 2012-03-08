/**
 * @brief A file containing implementation of noise injecting callback
 *   functions.
 *
 * A file containing implementation of callback functions for injecting noise.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2012-03-08
 * @version   0.1.4
 */

#include "noise.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

// Declarations of static functions (usable only within this module)
static const uint32_t initRNG();

namespace
{ // Static global variables (usable only within this module)
  boost::random::mt11213b g_rng; //!< A random number generator.
  PIN_LOCK g_rngLock; //!< A lock guarding the random number generator.
  const uint32_t g_rngSeed = initRNG(); // Initialise the generator at startup
}

/**
 * Initialises a random number generator.
 *
 * @return A seed used to initialise the random number generator.
 */
const uint32_t initRNG()
{
  // Initialise a lock used to guard the random number generator
  InitLock(&g_rngLock);

  // Return the seed used to initialise the random number generator
  return g_rng.default_seed;
}

/**
 * Generates a random frequency (number from 0 to 999).
 *
 * @return A number from 0 to 999.
 */
inline
uint32_t randomFrequency()
{
  // Restrict the generated number to a <0, 999> interval
  boost::random::uniform_int_distribution<> dist(0, 999);

  // Random number generation is not thread-safe, must be done exclusively
  GetLock(&g_rngLock, 1);
  // Critical section: generate a new random number from a <0, 999> interval
  uint32_t rn = dist(g_rng);
  // Do not hold the lock more time than is necessary to get maximum performance
  ReleaseLock(&g_rngLock);

  // Return the generated number from a <0, 999> interval
  return rn;
}

/**
 * Generates a random strength (number from 0 to @em max).
 *
 * @param max A number indicating the maximum strength which might be generated.
 * @return A number from 0 to @em max.
 */
inline
uint32_t randomStrength(UINT32 max)
{
  // Restrict the generated number to a <0, max> interval
  boost::random::uniform_int_distribution<> dist(0, max);

  // Random number generation is not thread-safe, must be done exclusively
  GetLock(&g_rngLock, 1);
  // Critical section: generate a new random number from a <0, max> interval
  uint32_t rn = dist(g_rng);
  // Do not hold the lock more time than is necessary to get maximum performance
  ReleaseLock(&g_rngLock);

  // Return the generated number from a <0, max> interval
  return rn;
}

/**
 * Injects a sleep noise to a program, e.g., sleeps for some amount of time.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A number of millisecond which the thread will be sleeping.
 */
VOID injectSleep(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // Inject noise (e.g. sleep for some time)
#ifdef DEBUG_NOISE_INJECTION
    CONSOLE("Sleeping for " + decstr(strength) + " miliseconds.\n");
#endif

    PIN_Sleep(strength);
  }
}

/**
 * Injects a yield noise to a program, e.g., gives up the CPU.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A number of times which the thread will give up the CPU.
 */
VOID injectYield(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // Inject noise (e.g. give up the CPU)
    for (unsigned int i = 0; i < strength; i++)
    { // Give up the CPU a specific number of times
#ifdef DEBUG_NOISE_INJECTION
      CONSOLE("Giving up the CPU for the " + decstr(i + 1) + "th time.\n");
#endif

      PIN_Yield();
    }
  }
}

/**
 * Injects a random sleep noise to a program, e.g., sleeps for a random amount
 *   of time.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A maximum number of millisecond which the thread will be
 *   sleeping.
 */
VOID injectRsSleep(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // Have to inject a random strength, not the maximum strength
    strength = randomStrength(strength);

#ifdef DEBUG_NOISE_INJECTION
    CONSOLE("Sleeping for " + decstr(strength) + " miliseconds.\n");
#endif

    // Inject the noise (e.g. sleep for some time)
    PIN_Sleep(strength);
  }
}

/**
 * Injects a random yield noise to a program, e.g., gives up the CPU.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A maximum number of times which the thread will give up the
 *   CPU.
 */
VOID injectRsYield(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // Have to inject a random strength, not the maximum strength
    strength = randomStrength(strength);

    for (unsigned int i = 0; i < strength; i++)
    { // Give up the CPU a specific number of times
#ifdef DEBUG_NOISE_INJECTION
      CONSOLE("Giving up the CPU for the " + decstr(i + 1) + "th time.\n");
#endif

      // Inject the noise (e.g. give up the CPU)
      PIN_Yield();
    }
  }
}

/** End of file noise.cpp **/
