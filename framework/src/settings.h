/**
 * @brief A file containing definitions of classes and function for handling
 *   settings.
 *
 * A file containing definitions of classes and function for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-11-07
 * @version   0.1.4
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <list>
#include <map>

#include <boost/regex.hpp>

#include "pin.H"

#include "mapper.h"

/**
 * @brief An enumeration describing the types of various functions.
 */
enum FunctionType
{
  FUNC_NORMAL, //!< A normal function (not related to thread synchronisation).
  FUNC_LOCK,   //!< A lock function.
  FUNC_UNLOCK  //!< An unlock function.
};

/**
 * @brief A structure describing a function.
 */
typedef struct FunctionDesc_s
{
  FunctionType type; //!< A type of a function.
  union
  { // Just to semantically differentiate the data (the code will be more clear)
    unsigned int lock; //!< An index of an object representing a lock.
  };
  unsigned int plvl; //!< A pointer level of an object (lock, condition, etc.).
  FuncArgMapper *farg; //!< An object mapping function arguments to unique IDs.

  /**
   * Constructs a FunctionDesc_s object.
   */
  FunctionDesc_s() : type(FUNC_NORMAL), lock(0), plvl(0), farg(NULL) {}

  /**
   * Constructs a FunctionDesc_s object.
   *
   * @param ft A type of a function.
   * @param idx An index of an object representing a lock.
   * @param pl A pointer level of the object representing the lock.
   * @param fam An object mapping function arguments to unique IDs.
   */
  FunctionDesc_s(FunctionType ft, unsigned int idx, unsigned int pl,
    FuncArgMapper *fam) : type(ft), lock(idx), plvl(pl), farg(fam) {}
} FunctionDesc;

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const FunctionDesc& value);

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;
typedef std::list< std::pair< std::string, boost::regex > > PatternList;
typedef std::map< std::string, FunctionDesc* > FunctionMap;

/**
 * @brief A class holding the ANaConDA framework settings.
 *
 * Holds the ANaConDA framework settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-11-07
 * @version   0.1.6.1
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
    /**
     * @brief A map containing names of functions for thread synchronisation.
     */
    FunctionMap m_syncFunctions;
  public: // Member methods for handling the ANaConDA framework settings
    void load();
    void print(std::ostream& s = std::cout);
  public: // Member methods for checking exclusions
    bool isExcludedFromInstrumentation(IMG image);
    bool isExcludedFromDebugInfoExtraction(IMG image);
  public: // Member methods for checking functions
    bool isSyncFunction(RTN rtn, FunctionDesc** desc = NULL);
  private: // Internal helper methods for loading parts of the settings
    void loadEnvVars();
    void loadExclusions();
    void loadSyncFunctions();
  private: // Internal helper methods
    std::string expandEnvVars(std::string s);
    std::string blobToRegex(std::string blob);
};

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
