/**
 * @brief Contains implementation of functions for accessing environment
 *   variables.
 *
 * A file containing implementation of functions for accessing environment
 *   variables.
 *
 * @file      env.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-18
 * @date      Last Update 2012-06-19
 * @version   0.1
 */

#include "env.h"

#include <assert.h>

#ifdef TARGET_WINDOWS
  #include <windows.h>
#else
  #include <boost/tokenizer.hpp>
#endif

void getEnvVars(EnvVarMap& envVars)
{
#ifdef TARGET_WINDOWS
  // Helper variables
  std::string envVar;
  LPTSTR nextEnvVar;
  LPTCH envVarBlock;
  size_t nvSepPos;

  // Get a block of data containing all environment variables
  if ((envVarBlock = GetEnvironmentStrings()) == NULL) return;

  // To free the block of data later, we need to preserve the pointer to it
  nextEnvVar = (LPTSTR)envVarBlock;

  while (*nextEnvVar)
  { // Process all environment variables
    envVar.assign(nextEnvVar);

    if (!envVar.empty() && envVar.at(0) != '=')
    { // Exclude unnamed environment variables
      nvSepPos = envVar.find('=');

      // All environment variables must have the name=value format here
      assert(nvSepPos != std::string::npos);

      // Save the name and value of an environment variable to the map
      envVars[envVar.substr(0, nvSepPos)] = envVar.substr(nvSepPos + 1);
    }

    // Move to the next environment variable in the block
    nextEnvVar += lstrlen(nextEnvVar) + 1;
  }

  // Free the block containing all environment variables
  FreeEnvironmentStrings(envVarBlock);
#else
  // Helper variables
  std::string name;

  // On POSIX systems, all environment variables are stored in this variable
  extern char **environ;

  // Type definitions
  typedef boost::tokenizer< boost::char_separator< char > > tokenizer;

  // Environment variables are stored in 'name=value' format
  boost::char_separator< char > sep("=");

  for (int i = 0; environ[i] != NULL; i++)
  { // Split each environment variable to 'name' and 'value' pairs
    tokenizer tokens(std::string(environ[i]), sep);

    // The first token must be the name of the environment variable
    name = *tokens.begin();

    // Use 'getenv' to get the value, 'environ' might have the values malformed
    envVars[name] = getenv(name.c_str());
  }
#endif
}

/** End of file env.cpp **/
