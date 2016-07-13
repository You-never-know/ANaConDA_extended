/**
 * @brief Contains definitions of classes and functions for handling settings.
 *
 * A file containing definitions of classes and functions for handling settings.
 *
 * @file      settings.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2016-07-13
 * @version   0.15.2
 */

#ifndef __PINTOOL_ANACONDA__SETTINGS_H__
  #define __PINTOOL_ANACONDA__SETTINGS_H__

#include <iostream>
#include <list>
#include <map>
#include <regex>
#include <set>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "pin.H"

#include "analyser.h"
#include "filter.h"
#include "mapper.h"
#include "noise.h"

#include "monitors/preds.hpp"
#include "monitors/svars.hpp"
#include "monitors/sync.hpp"

#include "utils/env.h"
#include "utils/writers.h"

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
 * @brief An enumeration of hook types supported by the framework.
 *
 * Enumerates types of functions the framework is able to monitor.
 */
typedef enum HookType_e
{
  HT_INVALID,       //!< An invalid function (hook).
  HT_LOCK,          //!< A function acquiring locks.
  HT_UNLOCK,        //!< A function releasing locks.
  HT_SIGNAL,        //!< A function signalling conditions.
  HT_WAIT,          //!< A function waiting for conditions.
  HT_LOCK_INIT,     //!< A function initialising locks.
  HT_GENERIC_WAIT,  //!< A function waiting for arbitrary objects.
  HT_THREAD_CREATE, //!< A function creating threads.
  HT_THREAD_INIT,   //!< A function initialising threads.
  HT_JOIN,          //!< A function joining two threads.
  HT_TX_START,      //!< A function starting transactions.
  HT_TX_COMMIT,     //!< A function committing transactions.
  HT_TX_ABORT,      //!< A function aborting transactions.
  HT_TX_READ,       //!< A function performing reads within transactions.
  HT_TX_WRITE,      //!< A function performing writes within transactions.
  HT_UNWIND,        //!< A function unwinding thread's stack.
  HT_DATA_FUNCTION, //!< A function working with some interesting data.
  HT_NOISE_POINT    //!< A function before which a noise should be inserted.
} HookType;

// Types of callback functions used by some of the hooks above
#define UNWIND_NO_RET 0
#define UNWIND_RETURN 1

// Forward type definitions
typedef struct HookInfo_s HookInfo;
typedef VOID (*HOOKINSTRUMENTFUNPTR)(RTN rtn, HookInfo* hi);

/**
 * @brief A structure containing information about a hook.
 *
 * Contains information about a function monitored by the framework.
 */
typedef struct HookInfo_s
{
  HookType type; //!< A type of function monitored by the framework.
  union
  { // Hook-type-specific data (each type of hook treats this data differently)
    int idx; //!< An index of an argument containing some interesting data.
    int lock; //!< An index of an argument representing a lock.
    int cond; //!< An index of an argument representing a condition.
    int thread; //!< An index of an argument representing a thread.
    int object; //!< An index of an argument representing an arbitrary object.
    int addr; //!< An index of an argument with the memory address read/written.
    int cbtype; //!< A type of callback function to be used by the hook.
  };
  /**
   * @brief A depth of a chain of pointers leading to some interesting data.
   *
   * As the mapper objects take a pointer to some interesting data, e.g., data
   *   representing a condition, a lock or a thread, as their parameter, the
   *   framework must be able to extract this data from a call to a function.
   *   However, the data might not be passed to a function directly, but using
   *   a chain of pointers, e.g., function(Data ***ptr). This variable tells
   *   the framework how many times an argument must be dereferenced to get to
   *   the interesting data, e.g., a condition, a lock or a thread.
   */
  unsigned int refdepth;
  FuncArgMapper* mapper; //!< An object mapping arbitrary data to unique IDs.
  HOOKINSTRUMENTFUNPTR instrument; //!< A function used to instrument the hook.
  void* data; //!< An arbitrary data assigned to the hook.

  /**
   * Constructs a HookInfo_s object.
   */
  HookInfo_s() : type(HT_INVALID), idx(0), refdepth(0), mapper(NULL),
    instrument(NULL), data(NULL) {}

  /**
   * Constructs a HookInfo_s object.
   *
   * @param t A type of function monitored by the framework.
   */
  HookInfo_s(HookType t) : type(t), idx(0), refdepth(0), mapper(NULL),
    instrument(NULL), data(NULL) {}

  /**
     * Constructs a HookInfo_s object.
     *
     * @param t A type of function monitored by the framework.
     * @param cbt A type of callback function to be used by the hook.
     */
    HookInfo_s(HookType t, int cbt) : type(t), cbtype(cbt), refdepth(0),
      mapper(NULL), instrument(NULL), data(NULL) {}

  /**
   * Constructs a HookInfo_s object.
   *
   * @param t A type of function monitored by the framework.
   * @param i An index of an argument containing some interesting data.
   * @param rd A depth of a chain of pointers leading to the interesting data.
   */
  HookInfo_s(HookType t, int i, unsigned int rd) : type(t), idx(i),
    refdepth(rd), mapper(NULL), instrument(NULL), data(NULL) {}

  /**
   * Constructs a HookInfo_s object.
   *
   * @param t A type of function monitored by the framework.
   * @param i An index of an argument containing some interesting data.
   * @param rd A depth of a chain of pointers leading to the interesting data.
   * @param m An object mapping the interesting data to unique IDs.
   */
  HookInfo_s(HookType t, int i, unsigned int rd, FuncArgMapper *m) : type(t),
    idx(i), refdepth(rd), mapper(m), instrument(NULL), data(NULL) {}
} HookInfo;

/**
 * @brief A structure containing information about a filter pattern (rule).
 */
typedef struct PatternInfo_s
{
  /**
   * @brief The original rule as found in the specification of the filter.
   */
  std::string rule;
  /**
   * @brief A blob pattern after resolving environment variables.
   */
  std::string blob;
  /**
   * @brief A string representation of the regular expression, i.e., the final
   *   pattern used for matching by the filter.
   */
  std::string pattern;
} PatternInfo;

// Definitions of functions for printing various data to a stream
std::ostream& operator<<(std::ostream& s, const HookInfo& value);

// Definitions of functions for concatenating various data with a string
std::string operator+(const std::string& s, const HookType& type);
std::string operator+(const HookType& type, const std::string& s);
std::string operator+(const char* s, const HookType& type);
std::string operator+(const HookType& type, const char* s);

// Type definitions
typedef std::set< std::string > BasicFilter;
typedef std::list< std::pair< std::string, std::regex > > PatternList;
typedef std::list< HookInfo* > HookInfoList;
typedef std::map< std::string, HookInfoList > HookInfoMap;
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
 * @date      Last Update 2016-07-13
 * @version   0.9.1
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

  typedef InvalidatingTreeFilter< PatternInfo > Filter;

  /**
   * @brief A structure containing filters supported by the framework.
   */
  typedef struct Filters_s
  {
    Filter access; //!< Memory access filter.
  } Filters;

  public: // Type definitions
    typedef VOID (*SETUPFUNPTR)(Settings* settings);
    typedef Filter::MatchResult FilterResult;
  private: // Static attributes
    static Settings* ms_instance; //!< A singleton instance.
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
     * @brief A structure containing filters supported by the framework.
     */
    Filters m_filters;
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
     * @brief A list of functions that should be excluded from monitoring.
     */
    BasicFilter m_excludedFunctions;
    /**
     * @brief A map containing information about all hooks.
     *
     * Contains information about all functions which the framework monitors.
     */
    HookInfoMap m_hooks;
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
  public: // Static methods
    static Settings* Get();
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

  public: // Member methods for checking exclusion filters
    bool disableMemoryAccessMonitoring(IMG image, FilterResult& reason);
    bool disableMemoryAccessMonitoring(RTN function, FilterResult& reason,
      FilterResult& imgReason);
  public: // Member methods for checking exclusions
    bool isExcludedFromInstrumentation(IMG image);
    bool isExcludedFromDebugInfoExtraction(IMG image);
    bool isExcludedFromMonitoring(RTN function);
  public: // Member methods for checking functions
    bool isHook(RTN rtn, HookInfoList** hl = NULL);
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
  public: // Member methods for obtaining information about files
    fs::path getConfigFile(fs::path path);
  public: // Member methods for obtaining information about hooks
    /**
     * Gets a list of hooks (functions monitored by the framework).
     *
     * @return A list of hooks (functions monitored by the framework).
     */
    HookInfoList getHooks()
    {
      HookInfoList hlist;

      BOOST_FOREACH(HookInfoMap::value_type& function, m_hooks)
      { // Process all functions monitored by the framework
        BOOST_FOREACH(HookInfo* hi, function.second)
        { // Each function can be monitored more than once
          hlist.push_back(hi);
        }
      }

      return hlist;
    }

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
    void loadFiltersFromFile(fs::path file, BasicFilter& filter);
    void loadHooks();
    void loadHooksFromFile(fs::path file, HookType type);
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
