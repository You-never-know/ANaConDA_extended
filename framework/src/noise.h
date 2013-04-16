/**
 * @brief Contains definitions of classes for handling noise injection.
 *
 * A file containing definitions of classes for registering noise injection
 *   functions and working with them.
 *
 * @file      noise.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2013-04-16
 * @version   0.1.3
 */

#ifndef __PINTOOL_ANACONDA__NOISE_H__
  #define __PINTOOL_ANACONDA__NOISE_H__

#include <iostream>
#include <map>

#include "pin.H"

// Type definitions
typedef VOID (*NOISEFUNPTR)(THREADID tid, UINT32 frequency, UINT32 strength);

/**
 * @brief A structure describing a noise.
 */
typedef struct NoiseDesc_s
{
  std::string type; //!< A type of a noise.
  NOISEFUNPTR function; //!< A noise injection function.
  unsigned int frequency; //!< A probability that a noise will be inserted.
  unsigned int strength; //!< A strength of a noise.
  /**
   * @brief A flag determining if the noise should be placed only before
   *   accesses to shared variables.
   */
  bool sharedVars;
  /**
   * @brief A flag determining if the noise should be placed only before
   *   instructions which have a predecessor.
   */
  bool predecessors;

  /**
   * Constructs a NoiseDesc_s object.
   */
  NoiseDesc_s() : type(), function(), frequency(0), strength(0),
    sharedVars(false), predecessors(false) {}

  /**
   * Constructs a NoiseDesc_s object.
   *
   * @param t A type of the noise.
   * @param f A probability that the noise will be inserted.
   * @param s A strength of the noise.
   */
  NoiseDesc_s(std::string t, unsigned int f, unsigned int s) : type(t),
    function(), frequency(f), strength(s), sharedVars(false),
    predecessors(false) {}

  /**
   * Constructs a NoiseDesc_s object.
   *
   * @param t A type of the noise.
   * @param f A probability that the noise will be inserted.
   * @param s A strength of the noise.
   * @param sv A flag determining if the noise should be placed only before
   *   accesses to shared variables.
   * @param p A flag determining if the noise should be placed only before
   *   instructions which have a predecessor.
   */
  NoiseDesc_s(std::string t, unsigned int f, unsigned int s, bool sv, bool p) :
    type(t), function(), frequency(f), strength(s), sharedVars(sv),
    predecessors(p) {}
} NoiseDesc;

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const NoiseDesc& value);

/**
 * @brief A class for registering and retrieving noise injection functions.
 *
 * Registers and provides noise injection functions.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2012-03-03
 * @version   0.1
 */
class NoiseFunctionRegister
{
  public: // Type definitions
    typedef std::map< std::string, NOISEFUNPTR > NoiseFunctionMap;
  private: // Static attributes
    static NoiseFunctionRegister* ms_instance; //!< A singleton instance.
  private: // Internal variables
    /**
     * @brief A map containing noise injection functions.
     */
    NoiseFunctionMap m_registeredNoiseFunctions;
  public: // Static methods
    static NoiseFunctionRegister* Get();
  public: // Member methods for registering and retrieving noise functions
    NOISEFUNPTR getFunction(std::string name);
    void registerFunction(std::string name, NOISEFUNPTR function);
};

// Macro definitions for simpler noise function registration and retrieval
#define REGISTER_NOISE_FUNCTION(name, function) \
  NoiseFunctionRegister::Get()->registerFunction(name, function)
#define GET_NOISE_FUNCTION(name) \
  NoiseFunctionRegister::Get()->getFunction(name)

#endif /* __PINTOOL_ANACONDA__NOISE_H__ */

/** End of file noise.h **/
