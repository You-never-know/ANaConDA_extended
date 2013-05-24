/**
 * @brief Contains definitions of classes and functions for handling settings.
 *
 * A file containing definitions of classes and functions for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2013-05-24
 * @version   0.7.0.2
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <list>
#include <map>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include "pin.H"

#include "analyser.h"
#include "mapper.h"
#include "noise.h"

#include "monitors/sync.h"
#include "monitors/svars.hpp"
#include "monitors/preds.h"

#include "util/env.h"
#include "util/writers.h"

// Namespace aliases
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::posix_time;

/**
 * @brief An enumeration of types of backtraces the framework is able to
 *   provide.
 */
typedef enum BacktraceType_e
{
  BT_NONE        = 0x0, //!< No backtraces.
  BT_LIGHTWEIGHT = 0x1, //!< Backtraces with return addresses.
  BT_FULL        = 0x2, //!< Backtraces with function names.
  BT_PRECISE     = 0x4  //!< Backtraces with call addresses.
} BacktraceType;

/**
 * @brief An enumeration determining the amount of information available in
 *   backtraces.
 */
typedef enum BacktraceVerbosity_e
{
  BV_MINIMAL  = 0x0, //!< Only the basic information.
  BV_DETAILED = 0x1, //!< All convenient information.
  BV_MAXIMAL  = 0x2  //!< All obtainable information.
} BacktraceVerbosity;

/**
 * @brief An enumeration determining which concurrent coverage information will
 *   the framework provide, if any.
 */
typedef enum ConcurrentCoverage_e
{
  CC_NONE  = 0x0, //!< No coverage information.
  CC_SYNC  = 0x1, //!< Synchronisation coverage.
  CC_SVARS = 0x2, //!< Shared variables.
  CC_PREDS = 0x4  //!< Predecessors.
} ConcurrentCoverage;

/**
 * @brief An enumeration of types of functions the framework is able to monitor.
 */
typedef enum FunctionType_e
{
  FUNC_NORMAL,        //!< A normal function.
  FUNC_LOCK,          //!< A lock function.
  FUNC_UNLOCK,        //!< An unlock function.
  FUNC_SIGNAL,        //!< A signal function.
  FUNC_WAIT,          //!< A wait function.
  FUNC_LOCK_INIT,     //!< A lock initialisation function.
  FUNC_GENERIC_WAIT,  //!< A generic wait function.
  FUNC_THREAD_CREATE, //!< A thread creation function.
  FUNC_THREAD_INIT,   //!< A thread initialisation function.
  FUNC_JOIN           //!< A join function.
} FunctionType;

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
std::ostream& operator<<(std::ostream& s, const FunctionDesc& value);

// Definitions of functions for concatenating various data with a string
std::string operator+(const std::string& s, const FunctionType& type);
std::string operator+(const FunctionType& type, const std::string& s);
std::string operator+(const char* s, const FunctionType& type);
std::string operator+(const FunctionType& type, const char* s);

// Type definitions
typedef std::list< std::pair< std::string, boost::regex > > PatternList;
typedef std::map< std::string, FunctionDesc* > FunctionMap;
typedef std::map< std::string, NoiseSettings* > NoiseSettingsMap;
typedef std::map< std::string, std::string > VarMap;

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
 * @date      Last Update 2013-05-21
 * @version   0.5
 */
class Settings
{
  /**
   * @brief A structure containing objects for monitoring various types of
   *   concurrent coverage.
   */
  typedef struct CoverageMonitors_s
  {
    SyncCoverageMonitor< FileWriter > sync; //!< Synchronisation coverage.
    SharedVariablesMonitor< FileWriter > svars; //!< Shared variables.
    PredecessorsMonitor< FileWriter > preds; //!< Predecessors.
  } CoverageMonitors;

  public: // Type definitions
    typedef VOID (*SETUPFUNPTR)(Settings* settings);
  private: // Retrieved variables
    /**
     * @brief A map containing values of environment variables.
     */
    EnvVarMap m_env;
    /**
     * @brief A time when the library started its execution.
     */
    pt::ptime m_timestamp = pt::microsec_clock::local_time();
    /**
     * @brief An integer used to initialise the random number generator.
     */
    UINT64 m_seed;
    /**
     * @brief A path to the ANaConDA framework's library.
     */
    fs::path m_library;
    /**
     * @brief A path to the analysed program.
     */
    fs::path m_program;
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
    NoiseSettingsMap m_noisePoints;
    /**
     * @brief A structure containing detailed information about a noise (type,
     *   frequency, strength) which should be inserted before each read from a
     *   memory.
     */
    NoiseSettings* m_readNoise;
    /**
      * @brief A structure containing detailed information about a noise (type,
      *   frequency, strength) which should be inserted before each write to a
      *   memory.
      */
    NoiseSettings* m_writeNoise;
    /**
      * @brief A structure containing detailed information about a noise (type,
      *   frequency, strength) which should be inserted before each atomic
      *   update of a memory.
      */
    NoiseSettings* m_updateNoise;
    /**
     * @brief A structure containing objects for monitoring various types of
     *   concurrent coverage.
     */
    CoverageMonitors m_coverage;
    /**
     * @brief An object representing the ANaConDA framework's library.
     */
    SharedLibrary* m_anaconda;
    /**
     * @brief A program analyser performing the analysis of the program which
     *   is the ANaConDA framework executing.
     */
    Analyser* m_analyser;
  private: // Registered callback functions
    /**
     * @brief A list of functions which will be called when the framework is
     *   being setup.
     */
    std::list< SETUPFUNPTR > m_onSetup;
  public: // Destructors
    ~Settings();
  public: // Member methods for handling the ANaConDA framework settings
    void load(int argc, char **argv) throw(SettingsError);
    void setup() throw(SettingsError);
    void print(std::ostream& s = std::cout);
  public: // Member methods for registering callback functions
    void registerSetupFunction(SETUPFUNPTR callback);
  public:
    /**
     * Gets a value of a configuration entry.
     *
     * @tparam A type of a value of a configuration entry.
     *
     * @param key A key identifying a configuration entry.
     * @return The value of the configuration entry.
     */
    template< typename T >
    const T& get(const std::string& key)
    {
      return m_settings[key].as< T >();
    }

  public: // Member methods for checking exclusions
    bool isExcludedFromInstrumentation(IMG image);
    bool isExcludedFromDebugInfoExtraction(IMG image);
  public: // Member methods for checking functions
    bool isSyncFunction(RTN rtn, FunctionDesc** fd = NULL);
    bool isNoisePoint(RTN rtn, NoiseSettings** ns = NULL);
  public: // Member methods for obtaining information about the analysed program
    std::string getProgramName();
    std::string getProgramPath();
  public: // Member methods for obtaining coverage configuration
    /**
     * Gets a structure containing objects for monitoring various types of
     *   concurrent coverage.
     *
     * @return A structure containing objects for monitoring various types of
     *   concurrent coverage.
     */
    CoverageMonitors& getCoverageMonitors() { return m_coverage; }
    std::string getCoverageFile(ConcurrentCoverage type);
  public: // Member methods for obtaining information about noise injection
    /**
     * Gets an integer used to initialise the random number generator.
     *
     * @return An integer used to initialise the random number generator.
     */
    UINT64 getSeed() { return m_seed; }

    /**
     * Gets a structure containing information about a noise which should be
     *   inserted before each read from a memory.
     *
     * @return A structure containing information about a noise which should be
     *   inserted before each read from a memory.
     */
    NoiseSettings* getReadNoise() { return m_readNoise; }

    /**
     * Gets a structure containing information about a noise which should be
     *   inserted before each write to a memory.
     *
     * @return A structure containing information about a noise which should be
     *   inserted before each write to a memory.
     */
    NoiseSettings* getWriteNoise() { return m_writeNoise; }

    /**
     * Gets a structure containing information about a noise which should be
     *   inserted before each atomic update of a memory.
     *
     * @return A structure containing information about a noise which should be
     *   inserted before each atomic update of a memory.
     */
    NoiseSettings* getUpdateNoise() { return m_updateNoise; }

  private: // Internal helper methods for loading parts of the settings
    void loadSettings(int argc, char **argv) throw(SettingsError);
    NoiseSettings* loadNoiseSettings(std::string prefix) throw(SettingsError);
    void loadEnvVars();
    void loadFilters();
    void loadFiltersFromFile(fs::path file, PatternList& list);
    void loadHooks();
    void loadHooksFromFile(fs::path file, FunctionType type);
    void loadAnalyser() throw(SettingsError);
  private: // Internal helper methods for setting up parts of the settings
    void setupNoise() throw(SettingsError);
    void setupCoverage() throw(SettingsError);
  private: // Internal helper methods
    std::string expandEnvVars(std::string s);
    std::string blobToRegex(std::string blob);
    VarMap getCoverageFilenameVariables(ConcurrentCoverage type);
    pt::ptime getLastTimestamp(ConcurrentCoverage type);
};

#endif /* __PINTOOL_ANACONDA__SETTINGS_H__ */

/** End of file settings.h **/
