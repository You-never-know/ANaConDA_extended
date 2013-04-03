/**
 * @brief A file containing implementation of noise injecting callback
 *   functions.
 *
 * A file containing implementation of callback functions for injecting noise.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2013-04-03
 * @version   0.3.3
 */

#include "noise.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "pin_die.h"

#include "../config.h"
#include "../noise.h"

/**
 * @brief An enumeration describing the types of noises.
 */
typedef enum NoiseType_e
{
  /**
   * @brief A noise causing a thread to sleep for some time.
   */
  SLEEP = 0x1,
  /**
   * @brief A noise causing a thread to give up CPU several times.
   */
  YIELD = 0x2,
  /**
   * @brief A noise causing a thread to loop in a cycle for some time.
   */
  BUSY_WAIT = 0x4,
  /**
   * @brief A noise causing a thread to perform several operations in a row
   *   while blocking the execution of all other threads.
   */
  INVERSE = 0x8
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

  INT32 g_tops; //!< A number of operations the running thread should perform.
  THREADID g_rtid; //!< An ID of a thread allowed to run while blocking other.
  PIN_SEMAPHORE g_continue; //!< A flag determining if other threads may run.
  PIN_RWMUTEX g_inSyncLock; //!< A lock syncing blocked and running threads.

  SharedVarsMonitor< FileWriter >* g_sVarsMon;
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
  while (true)
  { // We need to jump here sometimes, but goto is evil so we use while :D
    if (!PIN_SemaphoreIsSet(&g_continue))
    { // Inverse noise active, i.e., some thread is blocking all other threads
      PIN_RWMutexReadLock(&g_inSyncLock);

      if (PIN_SemaphoreIsSet(&g_continue))
      { // Inverse noise not active anymore, deactivated before we entered CS
        PIN_RWMutexUnlock(&g_inSyncLock);

        continue; // Jump to the beginning and check the continue flag again
      }

      if (tid == g_rtid)
      { // This is the only thread that may run, other threads are blocked
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": performing a single operation ("
          + decstr(g_tops - 1) + " operations remaining).\n");
#endif

        if (--g_tops < 1)
        { // We performed all operations for which we blocked the other threads
#if ANACONDA_PRINT_INJECTED_NOISE == 1
          CONSOLE("Thread " + decstr(tid) + ": resuming all threads.\n");
#endif

          PIN_SemaphoreSet(&g_continue); // Unblock the other (blocked) threads
        }

        PIN_RWMutexUnlock(&g_inSyncLock); // Allow inverse noise to be injected

        return; // Do not inject noise before the thread performing operations
      }
      else
      { // This is one of the blocked threads
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": blocked by thread "
          + decstr(g_rtid) + ", waiting.\n");
#endif

        if (!PIN_SemaphoreTimedWait(&g_continue, strength * 100))
        { // Time out reached, the running thread is likely waiting for some of
          // the blocked threads to do something and cannot continue until them
          // Recover from the deadlock as the injected noise probably caused it
#if ANACONDA_PRINT_INJECTED_NOISE == 1
          CONSOLE("Thread " + decstr(tid) + ": timeout, resuming all threads.\n");
#endif

          PIN_SemaphoreSet(&g_continue); // Unblock all (blocked) threads
        }
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        else
        { // Thread unblocked, continue running
          CONSOLE("Thread " + decstr(tid) + ": resumed.\n");
        }
#endif

        PIN_RWMutexUnlock(&g_inSyncLock); // Allow inverse noise to be injected

        continue; // Jump to the beginning and check the continue flag again
      }
    }
    else
    { // Inverse noise not active, i.e., threads may continue their execution
      if (NT & INVERSE)
      { // We might be injecting inverse noise, but we need to do it exclusively
        PIN_RWMutexWriteLock(&g_inSyncLock);

        if (!PIN_SemaphoreIsSet(&g_continue))
        { // Inverse noise already activated by another thread
          PIN_RWMutexUnlock(&g_inSyncLock);

          continue; // Jump to the beginning and check the continue flag again
        }
      }

      break; // Do not block the thread, continue and allow to inject noise
    }
  }

  if (randomFrequency() < frequency)
  { // We are under the frequency threshold, insert the noise
    if (ST & RANDOM)
    { // Need to convert the maximum strength to a random one
      strength = randomStrength(strength);
    }

    if (NT & SLEEP)
    { // Inject sleep noise, i.e. sleep for some time
#if ANACONDA_PRINT_INJECTED_NOISE == 1
      CONSOLE("Thread " + decstr(tid) + ": sleeping (" + decstr(strength)
        + " miliseconds).\n");
#endif

      PIN_Sleep(strength);
    }

    if (NT & YIELD)
    { // Inject yield noise, i.e. give up the CPU several times
      while (strength-- != 0)
      { // Strength determines how many times we should give up the CPU
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": giving up CPU (" + decstr(strength)
          + " times remaining).\n");
#endif

        PIN_Yield();
      }
    }

    if (NT & BUSY_WAIT)
    { // Inject busy wait noise, i.e., cycle in a loop for some time
      while (strength-- != 0)
      { // Strength determines how many loop iterations we should perform
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": looping (" + decstr(strength)
          + " iterations remaining).\n");
#endif

        frequency++; // No need to define new local variable, reuse frequency
      }
    }

    if (NT & INVERSE)
    { // Inject inverse noise, i.e., block all other threads for some time
#if ANACONDA_PRINT_INJECTED_NOISE == 1
      CONSOLE("Thread " + decstr(tid) + ": blocking all threads for the next "
        + decstr(strength) + " operations.\n");
#endif

      g_rtid = tid; // Only this thread is allowed to run, block all others
      g_tops = strength; // Block the threads for the next g_tops operations
      PIN_SemaphoreClear(&g_continue); // Block all other threads except this
    }
  }

  if (NT & INVERSE)
  { // Allow other threads to be blocked / inject inverse noise (if not active)
    PIN_RWMutexUnlock(&g_inSyncLock);
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
INSTANTIATE_NOISE_FUNCTION(BUSY_WAIT);
INSTANTIATE_NOISE_FUNCTION(INVERSE);

/**
 * Injects a noise to a program if accessing a shared variable.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param noiseDesc A structure containing the description of the noise which
 *   should be inserted before the shared variable.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 */
VOID injectSharedVariableNoise(THREADID tid, VOID* noiseDesc, ADDRINT addr,
  UINT32 size, ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Helper variables
  VARIABLE var;

  // Get information about the variable accessed
  DIE_GetVariable(rtnAddr, insAddr, addr, size, registers, /* input */
    var.name, var.type, &var.offset); /* output */

  if (g_sVarsMon->isSharedVariable(var))
  { // Inject a noise before the access to the shared variable
    static_cast< NoiseDesc* >(noiseDesc)->function(tid,
      static_cast< NoiseDesc* >(noiseDesc)->frequency,
      static_cast< NoiseDesc* >(noiseDesc)->strength);
  }
}

/**
 * Setups the access to shared variables storage and initialise synchronisation
 *   primitives used by the inverse noise.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupNoiseModule(Settings* settings)
{
  // Shared variable noise needs information about shared variables
  g_sVarsMon = &settings->getCoverageMonitors().svars;

  // A flag determining if threads may continue their execution
  PIN_SemaphoreInit(&g_continue);
  // At the beginning all threads may continue their execution
  PIN_SemaphoreSet(&g_continue);

  // A lock used to synchronise running and block threads
  PIN_RWMutexInit(&g_inSyncLock);
}

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
    "rs-" name, injectNoise< ntype, RANDOM >)

/**
 * Registers the ANaConDA framework's build-in noise injection functions.
 */
VOID registerBuiltinNoiseFunctions()
{
  REGISTER_BUILTIN_NOISE_FUNCTION("sleep", SLEEP);
  REGISTER_BUILTIN_NOISE_FUNCTION("yield", YIELD);
  REGISTER_BUILTIN_NOISE_FUNCTION("busy-wait", BUSY_WAIT);
  REGISTER_BUILTIN_NOISE_FUNCTION("inverse", INVERSE);
}

/** End of file noise.cpp **/
