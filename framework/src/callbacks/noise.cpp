/**
 * @brief A file containing implementation of noise injecting callback
 *   functions.
 *
 * A file containing implementation of callback functions for injecting noise.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2012-04-03
 * @version   0.2.0.2
 */

#include "noise.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../noise.h"

/**
 * @brief An enumeration describing the types of noises.
 */
typedef enum NoiseType_e
{
  SLEEP = 0x1, //!< A noise causing a thread to sleep for some time.
  YIELD = 0x2 //!< A noise causing a thread to give up CPU several times.
} NoiseType;

/**
 * @brief An enumeration describing the types of strength.
 */
typedef enum StrengthType_e
{
  FIXED = 0x1, //!< A strength which uses a concrete number as strength.
  RANDOM = 0x2 //!< A strength which uses a random number as strength.
} StrengthType;

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
 * Injects a noise to a program.
 *
 * @tparam NT A type of the noise which should be injected.
 * @tparam ST A type of the strength which should be used. If @c FIXED type is
 *   specified, a concrete strength given by the @em strength will be used, if
 *   @c RANDOM is used, a random strength between zero and a maximum strength
 *   given by the @em strength will be used.
 *
 * @param tid A number identifying the thread which will the noise influence.
 * @param frequency A probability that the noise will be injected (1000 = 100%).
 * @param strength A concrete or maximum strength of the noise.
 */
template< NoiseType NT, StrengthType ST >
inline
VOID injectNoise(THREADID tid, UINT32 frequency, UINT32 strength)
{
  if (randomFrequency() < frequency)
  { // We are under the frequency threshold, insert the noise
    if (ST & RANDOM)
    { // Need to convert the maximum strength to a random one
      strength = randomStrength(strength);
    }

    if (NT & SLEEP)
    { // Inject sleep noise, i.e. sleep for some time
#ifdef DEBUG_NOISE_INJECTION
      CONSOLE("Thread " + decstr(tid) + ": sleeping (" + decstr(strength)
        + " miliseconds).\n");
#endif

      PIN_Sleep(strength);
    }

    if (NT & YIELD)
    { // Inject yield noise, i.e. give up the CPU several times
      while (strength-- != 0)
      { // Strength determines how many times we should give up the CPU
#ifdef DEBUG_NOISE_INJECTION
        CONSOLE("Thread " + decstr(tid) + ": giving up CPU (" + decstr(strength)
          + " times remaining).\n");
#endif

        PIN_Yield();
      }
    }
  }
}

/**
 * Instantiates a concrete code of a noise injection function from a template.
 *
 * @note Instantiates one noise injection function for each strength type.
 */
#define INSTANTIATE_NOISE_FUNCTION(ntype) \
  template VOID PIN_FAST_ANALYSIS_CALL injectNoise< ntype, FIXED > \
    (THREADID tid, UINT32 frequency, UINT32 strength); \
  template VOID PIN_FAST_ANALYSIS_CALL injectNoise< ntype, RANDOM > \
    (THREADID tid, UINT32 frequency, UINT32 strength)

// Instantiate build-in noise injection functions
INSTANTIATE_NOISE_FUNCTION(SLEEP);
INSTANTIATE_NOISE_FUNCTION(YIELD);

/**
 * Registers the ANaConDA framework's build-in noise injection function.
 *
 * @note Registers one noise injection function for each strength type.
 *
 * @param name A name used to identify the noise injection function in the
 *   configuration files. The random strength version of the function will
 *   have a name with a @em rs- prefix (e.g. @em rs-sleep for @em sleep).
 * @param ntype A type of the the noise the noise function is injecting.
 */
#define REGISTER_BUILTIN_NOISE_FUNCTION(name, ntype) \
  NoiseFunctionRegister::Get()->registerFunction( \
    name, injectNoise< ntype, FIXED >); \
  NoiseFunctionRegister::Get()->registerFunction( \
    "rs-"name, injectNoise< ntype, RANDOM >)

/**
 * Registers the ANaConDA framework's build-in noise injection functions.
 */
VOID registerBuiltinNoiseFunctions()
{
  REGISTER_BUILTIN_NOISE_FUNCTION("sleep", SLEEP);
  REGISTER_BUILTIN_NOISE_FUNCTION("yield", YIELD);
}

/** End of file noise.cpp **/
