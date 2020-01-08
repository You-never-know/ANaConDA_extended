/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
