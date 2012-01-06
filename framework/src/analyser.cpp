/**
 * @brief Contains implementation of classes representing program analysers.
 *
 * A file containing implementation of classes representing program analysers.
 *
 * @file      analyser.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-01-06
 * @version   0.1.1
 */

#include "analyser.h"

/**
 * Constructs an Analyser object.
 *
 * @param shlib A shared library containing a program analyser.
 */
Analyser::Analyser(SharedLibrary* shlib) : m_shlib(shlib)
{
}

/**
 * Constructs an Analyser object from an existing Analyser object.
 *
 * @param a An object representing a program analyser.
 */
Analyser::Analyser(const Analyser& a) : m_shlib(new SharedLibrary(*a.m_shlib))
{
}

/**
 * Destroys an Analyser object.
 */
Analyser::~Analyser()
{
  delete m_shlib;
}

/**
 * Initialises a program analyser.
 */
void Analyser::init()
{
  // Type definitions
  typedef void (*init_func_t)();

  // Get a generic pointer to the init function in the program analyser
  void* symbol = m_shlib->resolve("init");

  if (symbol != NULL)
  { // If the init function is present in the program analyser, call it
    ((init_func_t)m_shlib->resolve("init"))();
  }
}

/**
 * Loads a program analyser.
 *
 * @param path A path to the program analyser.
 * @param error A reference to a string where the description of an error will
 *   be stored if the program analyser could not be loaded.
 * @return A pointer to an object representing the loaded program analyser or
 *   @em NULL if the program analyser could not be loaded.
 */
Analyser* Analyser::Load(fs::path path, std::string& error)
{
  // Load the shared library containing the program analyser
  SharedLibrary* shlib = SharedLibrary::Load(path, error);

  // If the shared library cannot be loaded, signal error
  if (shlib == NULL) return NULL;

  // If everything loaded successfully, return a new analyser object
  return new Analyser(shlib);
}

/** End of file analyser.cpp **/
