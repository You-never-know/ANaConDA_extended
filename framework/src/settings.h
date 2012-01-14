/**
 * @brief Contains definitions of classes and functions for handling settings.
 *
 * A file containing definitions of classes and functions for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2012-01-14
 * @version   0.1.14
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <list>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include "pin.H"

#include "analyser.h"
#include "mapper.h"

// Namespace aliases
namespace fs = boost::filesystem;
namespace po = boost::program_options;

/**
 * @brief An enumeration describing the types of various functions.
 */
enum FunctionType
{
  FUNC_NORMAL, //!< A normal function (not related to thread synchronisation).
  FUNC_LOCK,   //!< A lock function.
  FUNC_UNLOCK, //!< An unlock function.
  FUNC_SIGNAL, //!< A signal function.
  FUNC_WAIT    //!< A wait function.
};

/**
 * @brief An enumeration describing the types of noises.
 */
enum NoiseType
{
  NOISE_SLEEP, //!< A noise where a thread sleeps for some time.
  NOISE_YIELD  //!< A noise where a thread gives up a CPU some number of times.
};

/**
 * @brief A structure describing a noise.
 */
typedef struct NoiseDesc_s
{
  NoiseType type; //!< A type of a noise.
  unsigned int frequency; //!< A probability that a noise will be inserted.
  unsigned int strength; //!< A strength of a noise.

  /**
   * Constructs a NoiseDesc_s object.
   */
  NoiseDesc_s() : type(NOISE_SLEEP), frequency(0), strength(0) {}

  /**
   * Constructs a NoiseDesc_s object.
   *
   * @param nt A type of the noise.
   * @param f A probability that the noise will be inserted.
   * @param s A strength of the noise.
   */
  NoiseDesc_s(NoiseType nt, unsigned int f, unsigned int s) : type(nt),
    frequency(f), strength(s) {}
} NoiseDesc;

/**
 * @brief A structure describing a function.
 */
typedef struct FunctionDesc_s
{
  FunctionType type; //!< A type of a function.
  union
  { // Just to semantically differentiate the data (the code will be more clear)
    unsigned int lock; //!< An index of an object representing a lock.
    unsigned int cond; //!< An index of an object representing a condition.
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
   * @param idx An index of an object representing a lock or a condition.
   * @param pl A pointer level of the object representing the lock.
   * @param fam An object mapping function arguments to unique IDs.
   */
  FunctionDesc_s(FunctionType ft, unsigned int idx, unsigned int pl,
    FuncArgMapper *fam) : type(ft), lock(idx), plvl(pl), farg(fam) {}
} FunctionDesc;

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const NoiseDesc& value);
std::ostream& operator<<(std::ostream& s, const FunctionDesc& value);

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;
typedef std::list< std::pair< std::string, boost::regex > > PatternList;
typedef std::map< std::string, FunctionDesc* > FunctionMap;
typedef std::map< std::string, NoiseDesc* > NoiseMap;

/**
 * @brief A class representing an error in the ANaConDA framework's settings.
 *
 * Represents an error in the ANaConDA framework's settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-09
 * @date      Last Update 2011-12-09
 * @version   0.1
 */
class SettingsError : public std::exception
{
  private: // User-defined variables
    std::string m_msg;
  public: // Constructors
    SettingsError(const std::string& msg = "") throw();
    SettingsError(const SettingsError& se) throw();
  public: // Destructors
    virtual ~SettingsError() throw();
  public: // Inline virtual methods
    /**
     * Gets a C-style character string describing an error.
     *
     * @return A C-style character string describing the error.
     */
    virtual const char* what() const throw()
    {
      return m_msg.c_str();
    }
};

/**
 * @brief A class holding the ANaConDA framework's settings.
 *
 * Holds the ANaConDA framework's settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2012-01-14
 * @version   0.1.13
 */
class Settings
{
  private: // Retrieved variables
    EnvVarMap m_env; //!< A map containing values of environment variables.
    /**
     * @brief A path to the ANaConDA framework's library.
     */
    fs::path m_library;
    /**
     * @brief A map containing the ANaConDA framework's general settings.
     */
    po::variables_map m_settings;
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images which should not be instrumented.
     */
    PatternList m_insExclusions;
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images which should always be instrumented.
     */
    PatternList m_insInclusions;
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images whose debugging information should not be extracted.
     */
    PatternList m_dieExclusions;
    /**
     * @brief A list containing pairs of blob and regular expression patterns
     *   describing images whose debugging information should always be
     *   extracted.
     */
    PatternList m_dieInclusions;
    /**
     * @brief A map containing names of synchronisation functions before which
     *   and after which hooks signalling synchronisation actions will be
     *   inserted.
     */
    FunctionMap m_syncFunctions;
    /**
     * @brief A map containing names of functions before which a noise should
     *   be inserted. Each name is mapped to a structure containing detailed
     *   information about the noise (type, frequency, strength).
     */
    NoiseMap m_noisePoints;
    /**
     * @brief An object representing the ANaConDA framework's library.
     */
    SharedLibrary* m_anaconda;
    /**
     * @brief A program analyser performing the analysis of the program which
     *   is the ANaConDA framework executing.
     */
    Analyser* m_analyser;
  public: // Member methods for handling the ANaConDA framework settings
    void load(int argc, char **argv) throw(SettingsError);
    void print(std::ostream& s = std::cout);
  public: // Member methods for checking exclusions
    bool isExcludedFromInstrumentation(IMG image);
    bool isExcludedFromDebugInfoExtraction(IMG image);
  public: // Member methods for checking functions
    bool isSyncFunction(RTN rtn, FunctionDesc** desc = NULL);
    bool isNoisePoint(RTN rtn, NoiseDesc** desc = NULL);
  private: // Internal helper methods for loading parts of the settings
    void loadSettings(int argc, char **argv) throw(SettingsError);
    void loadEnvVars();
    void loadFilters();
    void loadFiltersFromFile(fs::path file, PatternList& list);
    void loadHooks();
    void loadHooksFromFile(fs::path file, FunctionType type);
    void loadAnalyser() throw(SettingsError);
  private: // Internal helper methods
    std::string expandEnvVars(std::string s);
    std::string blobToRegex(std::string blob);
};

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
