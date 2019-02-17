/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing implementation of noise injecting callback
 *   functions.
 *
 * A file containing implementation of callback functions for injecting noise.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-23
 * @date      Last Update 2019-02-05
 * @version   0.3.13
 */

#include "noise.h"

#include <boost/foreach.hpp>

#include "libdie-wrapper/pin_die.h"

#include "../config.h"
#include "../noise.h"

#include "../monitors/svars.hpp"

#include "../utils/random.hpp"
#include "../utils/scopedlock.hpp"

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
 * @brief An enumeration describing the types of shared variables filter.
 */
typedef enum SharedVariablesType_e
{
  SVT_ALL, //!< Inject a noise before any shared variable.
  SVT_ONE  //!< Inject a noise before one chosen shared variable.
} SharedVariablesType;

/**
 * @brief An enumeration describing the types of strength.
 */
typedef enum StrengthType_e
{
  FIXED = 0x1, //!< A strength which uses a concrete number as strength.
  RANDOM = 0x2 //!< A strength which uses a random number as strength.
} StrengthType;

/**
 * @brief An enumeration describing the types of instructions.
 */
typedef enum InstructionType_e
{
  IT_READ   = 0x0, //!< An instruction reading from a memory.
  IT_WRITE  = 0x1, //!< An instruction writing to a memory.
  IT_UPDATE = 0x2, //!< An instruction atomically updating a memory.
  IT_SYNC   = 0x3  //!< An instruction performing a synchronisation operation.
} InstructionType;

// Definitions of filter functions
typedef BOOL (*FILTERFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers);

/**
 * @brief A structure containing noise traits information.
 */
template < InstructionType IT >
struct NoiseTraits
{
};

/**
 * @brief Defines noise traits information for a specific type of instructions.
 *
 * @param instype A type of the instruction (items from the InstructionType
 *   enumeration).
 */
#define DEFINE_NOISE_TRAITS(instype) \
  template<> \
  struct NoiseTraits< instype > \
  { \
    typedef FILTERFUNPTR FilterType; \
    typedef std::vector< FilterType > FilterContainerType; \
    static FilterContainerType filters; \
  }; \
  NoiseTraits< instype >::FilterContainerType NoiseTraits< instype >::filters;

// Define traits information for the interesting types of instructions
DEFINE_NOISE_TRAITS(IT_READ);
DEFINE_NOISE_TRAITS(IT_WRITE);
DEFINE_NOISE_TRAITS(IT_UPDATE);

namespace
{ // Static global variables (usable only within this module)
  PIN_MUTEX g_timeLock; //!< A lock guarding access to local time.

  // Inverse noise configuration (and state) shared among all of the threads
  INT32 g_tops; //!< A number of operations the running thread should perform.
  THREADID g_rtid; //!< An ID of a thread allowed to run while blocking other.
  UINT32 g_timeout; //!< A maximum time the thread might block other threads.
  PIN_SEMAPHORE g_continue; //!< A flag determining if other threads may run.
  PIN_RWMUTEX g_inSyncLock; //!< A lock syncing blocked and running threads.

  // Information used by the shared variables filter
  SharedVariablesMonitor< FileWriter >* g_sVarsMon;
  /**
   * @brief A name of the only shared variable before which might be a noise
   *   injected.
   */
  std::string g_sharedVariable;
}

/**
 * Generates a random frequency, i.e., an integer number from 0 to 999.
 *
 * @return An integer number from 0 to 999.
 */
inline
UINT32 randomFrequency()
{
  return randomInt< UINT32 >(0, 999);
}

/**
 * Gets the current time.
 *
 * @return The current time.
 */
inline
pt::ptime getTime()
{
  // Boost implementation of local_time() might call functions from the C or C++
  // libraries, but C and C++ libraries linked into pintools are not thread-safe
  ScopedLock lock(g_timeLock);

  return pt::microsec_clock::local_time(); // Now we can safely access the time
}

/**
 * Generates a random strength, i.e., an integer number from 0 to @em max.
 *
 * @param max A number indicating the maximum strength which might be generated.
 * @return An integer number from 0 to @em max.
 */
inline
UINT32 randomStrength(UINT32 max)
{
  return randomInt< UINT32 >(0, max);
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
VOID PIN_FAST_ANALYSIS_CALL injectNoise(THREADID tid, UINT32 frequency,
  UINT32 strength)
{
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
      pt::ptime end = getTime() + pt::milliseconds(strength);

#if ANACONDA_PRINT_INJECTED_NOISE == 1
      pt::ptime now; // Helper variables

      while ((now = getTime()) < end)
#else
      while (getTime() < end)
#endif
      { // Strength determines how many loop iterations we should perform
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": looping ("
          + decstr((end - now).total_milliseconds())
          + " miliseconds remaining).\n");
#endif

        frequency++; // No need to define new local variable, reuse frequency
      }
    }

    if (NT & INVERSE)
    { // Inject inverse noise, i.e., block all other threads for some time
      ScopedWriteLock lock(g_inSyncLock);

      if (!PIN_SemaphoreIsSet(&g_continue))
      { // Some other thread already activated the inverse noise, do not inject
        // any noise and continue, we will be blocked by the inverse noise when
        // we reach the next location monitored (memory access, sync operation)
        return;
      }

#if ANACONDA_PRINT_INJECTED_NOISE == 1
      CONSOLE("Thread " + decstr(tid) + ": blocking all threads for the next "
        + decstr(strength) + " operations.\n");
#endif

      g_rtid = tid; // Only this thread is allowed to run, block all others
      g_tops = strength; // Block the threads for the next g_tops operations
      g_timeout = strength * 10; // Maximum time the threads might be blocked
      PIN_SemaphoreClear(&g_continue); // Block all other threads except this
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
INSTANTIATE_NOISE_FUNCTION(BUSY_WAIT);
INSTANTIATE_NOISE_FUNCTION(INVERSE);

/**
 * Injects a noise before an instruction performing a memory access if the noise
 *   filters allow it.
 *
 * @tparam IT A type of the instruction performing the memory access (might be
 *   an instruction reading, writing or atomically updating a memory).
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 * @param ns A structure containing the noise injection settings.
 */
template < InstructionType IT >
VOID injectAccessNoise(THREADID tid, ADDRINT addr, UINT32 size, ADDRINT rtnAddr,
  ADDRINT insAddr, CONTEXT* registers, NoiseSettings* ns)
{
  typedef NoiseTraits< IT > Traits; // Here are the filters we need stored

  for (typename Traits::FilterContainerType::iterator
    it = Traits::filters.begin(); it != Traits::filters.end(); it++)
  { // All filters must return true for the noise to be injected
    if (!(*it)(tid, addr, size, rtnAddr, insAddr, registers)) return;
  }

  // All filters evaluated to true, call the generator
  ns->generator(tid, ns->frequency, ns->strength);
}

/**
 * Allows to inject a noise only before accesses to shared variables.
 *
 * @tparam SVT A type of shared variables before which a noise might be placed.
 *   The noise might be placed before @em any shared variable (@c SVT_ALL) or
 *   @em one specific shared variable (@c SVT_ONE).
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 */
template< SharedVariablesType SVT >
inline
BOOL sharedVariablesFilter(THREADID tid, ADDRINT addr, UINT32 size,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Helper variables
  VARIABLE var;

  // Get information about the variable accessed
  DIE_GetVariable(rtnAddr, insAddr, addr, size, registers, /* input */
    var.name, var.type, &var.offset); /* output */

  if (SVT == SVT_ALL)
  { // Inject the noise before accesses to any shared variable
    return g_sVarsMon->isSharedVariable(var);
  }
  else
  { // Inject the noise before accesses to one specific shared variable
    return var.name == g_sharedVariable;
  }
}

/**
 * Allows to inject a noise only when the inverse noise is not active.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 */
BOOL inverseNoiseFilter(THREADID tid, ADDRINT addr, UINT32 size,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
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

        return false; // Do not inject noise before this thread
      }
      else
      { // This is one of the blocked threads
#if ANACONDA_PRINT_INJECTED_NOISE == 1
        CONSOLE("Thread " + decstr(tid) + ": blocked by thread "
          + decstr(g_rtid) + ", waiting.\n");
#endif

        if (!PIN_SemaphoreTimedWait(&g_continue, g_timeout))
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
      return true; // Do not block the thread and allow it to inject noise
    }
  }

  assert(false); // Something is very wrong if the control reaches this part
  return true; // Just to eliminate some warnings from dumb static analysers
}

/**
 * Setups noise filters for a specific type of instructions.
 *
 * @tparam IT A type of the instructions (might be instructions reading, writing
 *   or atomically updating a memory).
 *
 * @param noise A structure containing the information about the noise filters.
 */
template< InstructionType IT >
inline
VOID setupNoiseFilters(NoiseSettings* ns)
{
  typedef NoiseTraits< IT > Traits; // Here are the filters we need to setup

  if (ns->filters.size() > 0)
  { // Do not call the generator directly, call it through the filter function
    ns->filter = (AFUNPTR)injectAccessNoise< IT >;
  }

  BOOST_FOREACH(NoiseFilter filter, ns->filters)
  { // Configure all noise filters activated
    switch (filter)
    { // Each filter has to be configured separately
      case NF_SHARED_VARS: // Shared variables filter
        if (ns->properties.get< std::string >("svars.type") == "all")
        { // Inject noise before accesses to shared variables
          Traits::filters.push_back(sharedVariablesFilter< SVT_ALL >);
        }
        else
        { // Inject noise before accesses to one shared variable only
          Traits::filters.push_back(sharedVariablesFilter< SVT_ONE >);
        }
        break;
      case NF_PREDECESSORS: // Predecessors filter
        break;
      case NF_INVERSE_NOISE: // Inverse noise filter
        Traits::filters.push_back(inverseNoiseFilter);
        break;
      default: // Something is very wrong if the control reaches this part
        assert(false);
        break;
    }
  }
}

/**
 * Setups the access to shared variables storage and initialise synchronisation
 *   primitives used by the inverse and busy wait noise.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupNoiseModule(Settings* settings)
{
  // Shared variable noise needs information about shared variables
  g_sVarsMon = &settings->getCoverageMonitors().svars;

  // TODO: choose the shared variable only when needed
  std::vector< std::string > svars = g_sVarsMon->getSharedVariables();

  if (!svars.empty())
  { // Randomly choose one of the shared variables detected in previous runs
    g_sharedVariable = svars.at(randomStrength(svars.size() - 1));
  }

  // Setup the noise placement filters for each type of memory accesses
  setupNoiseFilters< IT_READ >(settings->getReadNoise());
  setupNoiseFilters< IT_WRITE >(settings->getWriteNoise());
  setupNoiseFilters< IT_UPDATE >(settings->getUpdateNoise());

  // A flag determining if threads may continue their execution
  PIN_SemaphoreInit(&g_continue);
  // At the beginning all threads may continue their execution
  PIN_SemaphoreSet(&g_continue);

  // A lock used to synchronise running and block threads
  PIN_RWMutexInit(&g_inSyncLock);

  // A lock guarding access to local time
  PIN_MutexInit(&g_timeLock);
}

/**
 * Registers the ANaConDA framework's build-in noise injection generators.
 *
 * @note Registers one noise injection generator for each strength type.
 *
 * @param name A name used to identify the noise injection generator in the
 *   configuration files. The random strength version of the generator will
 *   have a name with a @em rs- prefix (e.g. @em rs-sleep for @em sleep).
 * @param ntype A type of the the noise the noise function is injecting.
 */
#define REGISTER_BUILTIN_NOISE_GENERATOR(name, ntype) \
  NoiseGeneratorRegister::Get()->registerNoiseGenerator( \
    name, injectNoise< ntype, FIXED >); \
  NoiseGeneratorRegister::Get()->registerNoiseGenerator( \
    "rs-" name, injectNoise< ntype, RANDOM >)

/**
 * Registers the ANaConDA framework's build-in noise injection functions.
 */
VOID registerBuiltinNoiseFunctions()
{
  REGISTER_BUILTIN_NOISE_GENERATOR("sleep", SLEEP);
  REGISTER_BUILTIN_NOISE_GENERATOR("yield", YIELD);
  REGISTER_BUILTIN_NOISE_GENERATOR("busy-wait", BUSY_WAIT);
  REGISTER_BUILTIN_NOISE_GENERATOR("inverse", INVERSE);
}

/** End of file noise.cpp **/
