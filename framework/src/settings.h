/**
 * @brief A file containing definitions of classes and function for handling
 *   settings.
 *
 * A file containing definitions of classes and function for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-27
 * @version   0.1.2.3
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <list>
#include <map>

#include <boost/regex.hpp>

#include "pin.H"

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;
typedef std::list< std::pair< std::string, boost::regex > > PatternList;

/**
 * @brief A class holding the ANaConDA framework settings.
 *
 * Holds the ANaConDA framework settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-10-27
 * @version   0.1.4
 */
class Settings
{
  private: // Retrieved variables
    EnvVarMap m_env; //!< A map containing values of environment variables.
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images which should not be instrumented.
     */
    PatternList m_insExclusions;
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images whose debugging information should not be extracted.
     */
    PatternList m_dieExclusions;
  public: // Member methods for handling the ANaConDA framework settings
    void load();
    void print(std::ostream& s = std::cout);
  public: // Member methods for checking exclusions
    bool isExcludedFromInstrumentation(IMG image);
    bool isExcludedFromDebugInfoExtraction(IMG image);
  private: // Internal helper methods for loading parts of the settings
    void loadEnvVars();
    void loadExclusions();
  private: // Internal helper methods
    std::string expandEnvVars(std::string s);
    std::string blobToRegex(std::string blob);
};

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
