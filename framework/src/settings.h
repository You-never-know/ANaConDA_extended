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
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <map>

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;

/**
 * @brief A class holding the ANaConDA framework settings.
 *
 * Holds the ANaConDA framework settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-20
 * @version   0.1
 */
class Settings
{
  private: // Retrieved variables
    EnvVarMap m_env; //!< A map containing values of environment variables.
  public: // Member methods for handling the ANaConDA framework settings
    void load();
    void print(std::ostream& s = std::cout);
  private: // Internal helper methods
    void loadEnvVars();
};

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
