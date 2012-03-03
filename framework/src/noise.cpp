/**
 * @brief Contains implementation of classes for handling noise injection.
 *
 * A file containing implementation of classes for registering noise injection
 *   functions and working with them.
 *
 * @file      noise.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-03
 * @date      Last Update 2012-03-03
 * @version   0.1.1
 */

#include "noise.h"

/**
 * Prints a noise description to a stream.
 *
 * @param s A stream to which the noise description should be printed.
 * @param value A noise description.
 * @return The stream to which was the noise description printed.
 */
std::ostream& operator<<(std::ostream& s, const NoiseDesc& value)
{
  // The parameters of the noise are always the same, only type may differ
  s << value.type << "(" << value.frequency << "," << value.strength << ")";

  // Return the stream to which was the noise description printed
  return s;
}

// Initialisation of the singleton instance
NoiseFunctionRegister* NoiseFunctionRegister::ms_instance = NULL;

/**
 * Gets a singleton instance.
 *
 * @note If no singleton instance exist, the method will create one.
 *
 * @return The singleton instance.
 */
NoiseFunctionRegister* NoiseFunctionRegister::Get()
{
  if (ms_instance == NULL)
  { // No singleton instance yet, create one
    ms_instance = new NoiseFunctionRegister();
  }

  return ms_instance;
}

/**
 * Gets a noise injection function.
 *
 * @param name A name identifying the noise injection function.
 * @return A noise injection function or @em NULL if no noise injection function
 *   with the specified name is registered.
 */
NOISEFUNPTR NoiseFunctionRegister::getFunction(std::string name)
{
  // Try to find a noise injection function with the specified name
  NoiseFunctionMap::iterator it = m_registeredNoiseFunctions.find(name);

  if (it != m_registeredNoiseFunctions.end())
  { // Noise injection function found, return it
    return it->second;
  }

  // No noise injection function found
  return NULL;
}

/**
 * Registers a noise injection function.
 *
 * @warning If a noise injection function with the specified name already exist,
 *   it will be overwritten by the specified noise injection function.
 *
 * @param name A name identifying the noise injection function.
 * @param function A noise injection function.
 */
void NoiseFunctionRegister::registerFunction(std::string name,
  NOISEFUNPTR function)
{
  m_registeredNoiseFunctions[name] = function;
}

/** End of file noise.cpp **/
