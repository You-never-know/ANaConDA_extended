/**
 * @brief A file containing implementation of classes and function for handling
 *   settings.
 *
 * A file containing implementation of classes and function for handling
 *   settings.
 *
 * @file      settings.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-20
 * @version   0.1.1
 */

#include "settings.h"

#ifdef TARGET_LINUX
  #include <boost/tokenizer.hpp>
#endif

/**
 * Loads the ANaConDA framework settings.
 */
void Settings::load()
{
  // Load environment variables (might be referenced later)
  this->loadEnvVars();
}

/**
 * Prints the ANaConDA framework settings.
 *
 * @param s A stream to which should be the settings printed. If no stream is
 *   specified, standard output stream will be used.
 */
void Settings::print(std::ostream& s)
{
  // Helper variables
  EnvVarMap::iterator it;

  // Print a section containing loaded environment variables
  s << "Environment variables\n---------------------\n";

  for (it = m_env.begin(); it != m_env.end(); it++)
  { // Print each environment variable
    s << it->first << "=" << it->second << std::endl;
  }
}

/**
 * Loads values of environment variables.
 */
void Settings::loadEnvVars()
{
#ifdef TARGET_LINUX
  // Helper variables
  std::string name, value;

  // On POSIX systems, all environment variables are stored in this variable
  extern char **environ;

  // Type definitions
  typedef boost::tokenizer< boost::char_separator< char > > tokenizer;

  // Environment variables are stored in 'name=value' format
  boost::char_separator< char > sep("=");

  for (int i = 0; environ[i] != NULL; i++)
  { // Split each environment variable to 'name' and 'value' pairs
    tokenizer tokens(std::string(environ[i]), sep);

    // Access the parts of the environment variable
    tokenizer::iterator it = tokens.begin();

    name = *it++; // First part is the name of the environment variable
    value = *it; // Second part is the value of the environment variable

    // Save the environment variable to the settings object
    m_env[name] = value;
  }
#endif
}

/** End of file settings.cpp **/
