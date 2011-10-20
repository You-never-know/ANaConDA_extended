/**
 * @brief A file containing definitions of classes and function for handling
 *   settings.
 *
 * A file containing definitions of classes and function for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-20
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <map>

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;

/**
 * @brief A structure holding the ANaConDA framework settings.
 *
 * Holds the ANaConDA framework settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-20
 * @version   0.1
 */
typedef struct Settings_s
{
  /**
   * @brief A map containing values of environment variables.
   */
  EnvVarMap env;
} Settings;

void loadEnvVars(Settings& settings);

void loadSettings(Settings& settings);

void printSettings(Settings& settings, std::ostream& s = std::cout);

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
