/**
 * @brief Contains implementation of classes and functions for handling
 *   settings.
 *
 * A file containing implementation of classes and functions for handling
 *   settings.
 *
 * @file      settings.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2016-03-30
 * @version   0.12
 */

#include "settings.h"

#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "defs.h"

#ifdef TARGET_LINUX
  #include "utils/linux/dlutils.h"
  #include "utils/linux/elfutils.h"
#endif

// Helper macros used in the whole module
#define FORMAT_STR(frmt, args) \
  (boost::format(frmt) % args).str()

#ifdef BOOST_NO_EXCEPTIONS
// Exceptions cannot be used so we must define the throw_exception() manually
namespace boost { void throw_exception(std::exception const& e) { return; } }
#endif

// Pintools on Windows cannot use exceptions as they would interfere with the
// exception handling in PIN, so print the error and exit on Windows for now
#ifdef TARGET_WINDOWS
  #define SETTINGS_ERROR(error) \
    do { \
      CONSOLE_NOPREFIX("error: " + std::string(error) + "\n"); \
      exit(EXIT_FAILURE); \
    } while (false)
#else
  #define SETTINGS_ERROR(error) throw SettingsError(error)
#endif

/**
 * @brief An array containing textual representation of hook types, i.e., of the
 *   @c HookType enumeration constants).
 */
const char* g_hookTypeString[] = {
  "invalid hook",
  "lock function",
  "unlock function",
  "signal function",
  "wait function",
  "lock initialisation function",
  "generic wait function",
  "thread creation function",
  "thread initialisation function",
  "join function",
  "start transaction function",
  "commit transaction function",
  "abort transaction function",
  "transactional read function",
  "transactional write function",
  "noise point function"
};

/**
 * @brief An array holding a text description of the concurrent coverage
 *   information the framework can provide (a text description of the
 *   @c ConcurrentCoverage enumeration constants).
 */
const char* g_concurrentCoverageString[] = {
  "none",
  "synchronisation",
  "sharedvars",
  "",
  "predecessors"
};

/**
 * @brief An array holding a short text description of the concurrent coverage
 *   information the framework can provide (a shorter text description of the
 *   @c ConcurrentCoverage enumeration constants).
 */
const char* g_concurrentCoverageShortString[] = {
  "none",
  "sync",
  "svars",
  "",
  "preds"
};

/**
 * Prints a section containing a list of (inclusion or exclusion) patterns to
 *   a stream.
 *
 * @param s A stream to which should be the section printed.
 * @param title A title of the section.
 * @param list A list of patterns contained in the section.
 */
inline
void printFilters(std::ostream& s, const char *title, PatternList& list)
{
  // Print a section with the specified title containing loaded patterns
  s << "\n" << title << "\n" << std::string(strlen(title), '-') << "\n";

  for (PatternList::iterator it = list.begin(); it != list.end(); it++)
  { // Print all blob patterns in the list (no need to print regex patterns)
    s << it->first << std::endl;
  }
}

/**
 * Checks if an image is excluded from some operation.
 *
 * @note The included images have a higher priority than the excluded ones,
 *   i.e., exclusion of an image might be prevented by including the image.
 *   This allows one first to exclude a set of images and then include some
 *   specific image to prevent it from the exclusion.
 *
 * @param image An image.
 * @param excludes A list describing excluded images.
 * @param includes A list describing included images.
 * @return @em True if the image is excluded, @em false if the image is not
 *   excluded at all or is excluded, but this exclusion was prevented by an
 *   include rule.
 */
inline
bool isExcluded(IMG image, PatternList& excludes, PatternList& includes)
{
  // Helper variables
  PatternList::iterator it;

  // Extract the name of the image (should be a file name which can be matched)
  std::string name = IMG_Name(image);

  for (it = excludes.begin(); it != excludes.end(); it++)
  { // Try to match the file name to any of the exclusion patterns
    if (regex_match(name, it->second))
    { // The image should be excluded, but include might prevent this
      for (it = includes.begin(); it != includes.end(); it++)
      { // Try to match the file name to any of the inclusion patterns
        if (regex_match(name, it->second)) return false;
      }

      // No inclusion pattern forced the excluded image to be included back
      return true;
    }
  }

  // No pattern matches the file name, the image is not excluded
  return false;
}

/**
 * Expands all variables in a string.
 *
 * @param s A string containing references to one or more variables.
 * @param vars A table mapping the names of the variables to their respective
 *   values.
 * @param seps A two-character array holding the characters which enclose the
 *   names of the variables. The default value is '{}' which means that the
 *   names of the variables are enclosed in curly brackets, e.g. {variable}.
 * @return A string with all variables replaced by their values.
 */
std::string expandVars(std::string s, VarMap vars, const char seps[2] = "{}")
{
  // Helper variables
  std::string expanded;
  std::string::iterator it = s.begin();

  while (it != s.end())
  { // Search the whole string for references to variables
    if (*it == seps[0])
    { // Beginning of the specification of a name of a variable
      std::string name;

      while (++it != s.end())
      { // Get the name of the variable
        if (*it == seps[1])
        { // Valid name specification
          break;
        }
        else
        { // Part of the name specification
          name += *it;
        }
      }

      if (it == s.end())
      { // Name specification incomplete, for now ignore and keep the text
        return expanded + seps[0] + name;
      }

      // Helper variables
      VarMap::iterator varIt;

      if ((varIt = vars.find(name)) != vars.end())
      { // Referenced existing variable, replace it with its value
        expanded += varIt->second;
      }
      else
      { // Referenced non-existent variable, cannot expand it
        expanded += "${" + name + "}";
      }
    }
    else
    { // Other characters should just be copied to the expanded string
      expanded += *it;
    }

    it++; // Move to the next character in the string
  }

  return expanded; // Return the string with expanded variables
}

/**
 * Prints information about a hook (monitored function) to a stream.
 *
 * @param s A stream to which information about the hook should be printed.
 * @param value A structure containing information about the hook.
 * @return The stream to which was information about the hook printed.
 */
std::ostream& operator<<(std::ostream& s, const HookInfo& value)
{
  // Every hook has a type, so first print the type
  s << g_hookTypeString[value.type] << "(";

  if ((HT_INVALID < value.type && value.type < HT_TX_START)
    || (HT_TX_READ <= value.type && value.type <= HT_TX_WRITE))
  { // Sync and transactional memory access functions have index and refdepth
    s << "idx=" << value.idx << ",refdepth=" << value.refdepth;
  }

  if (HT_INVALID < value.type && value.type < HT_TX_START)
  { // Sync functions also have a mapper object assigned
    s << ",mapper=" << hex << value.mapper << dec;
  }

  // Finish formatting the information and return the stream used
  return s << ")";
}

/**
 * Concatenates a string with a type of a function the framework is able to
 *   monitor.
 *
 * @param s A string.
 * @param type A type of a function the framework is able to monitor.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em type.
 */
std::string operator+(const std::string& s, const HookType& type)
{
  return s + g_hookTypeString[type];
}

/**
 * Concatenates a type of a function the framework is able to monitor with a
 *   string.
 *
 * @param type A type of a function the framework is able to monitor.
 * @param s A string.
 * @return A new string with a value of a string representation of @em type
 *   followed by @em s.
 */
std::string operator+(const HookType& type, const std::string& s)
{
  return g_hookTypeString[type] + s;
}

/**
 * Concatenates a C string with a type of a function the framework is able to
 *   monitor.
 *
 * @param s A C string.
 * @param type A type of a function the framework is able to monitor.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em type.
 */
std::string operator+(const char* s, const HookType& type)
{
  return std::string(s) + g_hookTypeString[type];
}

/**
 * Concatenates a type of a function the framework is able to monitor with a C
 *   string.
 *
 * @param type A type of a function the framework is able to monitor.
 * @param s A C string.
 * @return A new string with a value of a string representation of @em type
 *   followed by @em s.
 */
std::string operator+(const HookType& type, const char* s)
{
  return std::string(g_hookTypeString[type]) + s;
}

/**
 * Constructs a SettingsError object.
 *
 * @param msg A message describing an error in the ANaConDA framework's
 *   settings.
 */
SettingsError::SettingsError(const std::string& msg) throw() : m_msg(msg)
{
}

/**
 * Constructs a SettingsError object from an existing SettingsError object.
 *
 * @param se An object representing an error in the ANaConDA framework's
 *   settings.
 */
SettingsError::SettingsError(const SettingsError& se) throw() : m_msg(se.m_msg)
{
}

/**
 * Destroys a SettingsError object.
 */
SettingsError::~SettingsError() throw()
{
}

// Initialisation of the singleton instance
Settings* Settings::ms_instance = NULL;

/**
 * Gets a singleton instance.
 *
 * @note If no singleton instance exist, the method will create one.
 *
 * @return The singleton instance.
 */
Settings* Settings::Get()
{
  if (ms_instance == NULL)
  { // No singleton instance yet, create one
    ms_instance = new Settings();
  }

  return ms_instance;
}

/**
 * Destroys a Settings object.
 */
Settings::~Settings()
{
  // Shut down the analyser (e.g. execute its finalisation code)
  m_analyser->finish();

  // Close the output file used by the synchronisation coverage monitor
  m_coverage.sync.close();
}

/**
 * Loads the ANaConDA framework's settings.
 *
 * @param argc A number of arguments passed to the PIN run script.
 * @param argv A list of arguments passed to the PIN run script.
 * @throw SettingsError if the settings contain errors.
 */
void Settings::load(int argc, char **argv) throw(SettingsError)
{
  // Load environment variables (might be referenced later)
  this->loadEnvVars();

  // Load general settings (from command line and config file)
  this->loadSettings(argc, argv);

  // Load patterns describing included and excluded images
  this->loadFilters();

  // Load names of functions acting as hooks in the program
  this->loadHooks();

  // Load a program analyser able to analyse the program
  this->loadAnalyser();
}

/**
 * Setups the ANaConDA framework.
 *
 * @throw SettingsError if the settings contain errors.
 */
void Settings::setup() throw(SettingsError)
{
  // Setup the noise settings
  this->setupNoise();

  // Setup the coverage monitoring settings
  this->setupCoverage();

  for (std::list< SETUPFUNPTR >::iterator it = m_onSetup.begin();
    it != m_onSetup.end(); it++)
  { // Notify other parts of the framework that it is being setup
    (*it)(this);
  }
}

/**
 * Prints the ANaConDA framework's settings.
 *
 * @param s A stream to which should be the settings printed. If no stream is
 *   specified, standard output stream will be used.
 */
void Settings::print(std::ostream& s)
{
  // Helper variables
  EnvVarMap::iterator envIt;
  HookInfoMap::iterator hIt;
  NoiseSettingsMap::iterator nsIt;

  // Helper macros used only in this method
  #define PRINT_OPTION(name, type) \
    if (m_settings.count(name)) \
      s << name << " = " << m_settings[name].as< type >() << "\n";
  #define PRINT_NOISE_OPTION(prefix) \
    PRINT_OPTION(prefix".filters", std::string) \
    PRINT_OPTION(prefix".filters.sharedvars.type", std::string) \
    PRINT_OPTION(prefix".filters.sharedvars.file", std::string) \
    PRINT_OPTION(prefix".filters.predecessors.file", std::string) \
    PRINT_OPTION(prefix".type", std::string) \
    PRINT_OPTION(prefix".frequency", int) \
    PRINT_OPTION(prefix".strength", int)
  #define PRINT_SETTING(name, value) \
    s << name << " = " << value << "\n";

  // Print the ANaConDA framework settings
  s << "Settings\n"
    << "--------\n";

  // Print a section containing loaded general settings
  s << "\nGeneral settings"
    << "\n----------------\n";

  PRINT_OPTION("config", fs::path);
  PRINT_OPTION("analyser", fs::path);
  PRINT_OPTION("debug", std::string);
  PRINT_OPTION("seed", UINT64);
  PRINT_OPTION("backtrace.type", std::string);
  PRINT_OPTION("backtrace.verbosity", std::string);
  PRINT_OPTION("coverage.synchronisation", bool);
  PRINT_OPTION("coverage.sharedvars", bool);
  PRINT_OPTION("coverage.predecessors", bool);
  PRINT_OPTION("coverage.filename", std::string);
  PRINT_OPTION("coverage.directory", fs::path);
  PRINT_NOISE_OPTION("noise");
  PRINT_NOISE_OPTION("noise.read");
  PRINT_NOISE_OPTION("noise.write");
  PRINT_NOISE_OPTION("noise.update");

  // Print a section containing internal settings
  s << "\nInternal settings"
    << "\n-----------------\n";

  PRINT_SETTING("timestamp", pt::to_iso_string(m_timestamp));
  PRINT_SETTING("seed", m_seed);

  // Print a section containing loaded environment variables
  s << "\nEnvironment variables"
    << "\n---------------------\n";

  for (envIt = m_env.begin(); envIt != m_env.end(); envIt++)
  { // Print each environment variable
    s << envIt->first << "=" << envIt->second << std::endl;
  }

  // Print a section containing loaded instrumentation exclusion patterns
  printFilters(s, "Images which will not be instrumented", m_insExclusions);

  // Print a section containing loaded instrumentation inclusion patterns
  printFilters(s, "Images which will be always instrumented", m_insInclusions);

  // Print a section containing loaded debug info extraction exclusion patterns
  printFilters(s, "Images whose debugging information will not be extracted",
    m_dieExclusions);

  // Print a section containing loaded debug info extraction inclution patterns
  printFilters(s, "Images whose debugging information will always be extracted",
    m_dieInclusions);

  // Print a section containing information about hooks (monitored functions)
  s << "\nHooks (monitored functions)"
    << "\n---------------------------\n";

  for (hIt = m_hooks.begin(); hIt != m_hooks.end(); hIt++)
  { // Print information about a hook
    s << hIt->first << " [" << *hIt->second;

    if ((nsIt = m_noisePoints.find(hIt->first)) != m_noisePoints.end())
    { // The hook is also a noise point
      s << ",noise point(generator=" << *nsIt->second << ")";
    }

    s << "]" << std::endl;
  }

  // Print a section containing information about noise points
  s << "\nNoise points (functions before which noise might be generated)"
    << "\n--------------------------------------------------------------\n";

  for (nsIt = m_noisePoints.begin(); nsIt != m_noisePoints.end(); nsIt++)
  { // Print the names of noise points with the description of the noise
    s << nsIt->first << " [" << *nsIt->second << "]" << std::endl;
  }
}

/**
 * Registers a function which will be called when the framework is being setup.
 *
 * @param callback A function to be called when the framework is being setup.
 */
void Settings::registerSetupFunction(SETUPFUNPTR callback)
{
  m_onSetup.push_back(callback);
}

/**
 * Checks if an image is excluded from instrumentation.
 *
 * @param image An image.
 * @return @em True if the image is excluded, @em false if the image is not
 *   excluded at all or is excluded, but this exclusion was prevented by an
 *   include rule.
 */
bool Settings::isExcludedFromInstrumentation(IMG image)
{
  return isExcluded(image, m_insExclusions, m_insInclusions);
}

/**
 * Checks if an image is excluded from debugging information extraction.
 *
 * @param image An image.
 * @return @em True if the image is excluded, @em false if the image is not
 *   excluded at all or is excluded, but this exclusion was prevented by an
 *   include rule.
 */
bool Settings::isExcludedFromDebugInfoExtraction(IMG image)
{
  return isExcluded(image, m_dieExclusions, m_dieInclusions);
}

/**
 * Checks if a function is a hook (monitored function).
 *
 * @param rtn An object representing the function.
 * @param hi If specified and not @em NULL, a pointer to a structure containing
 *   the information about the hook will be stored here.
 * @return @em True if the function is a hook (monitored function), @em false
 *   otherwise.
 */
bool Settings::isHook(RTN rtn, HookInfo** hi)
{
  // Determine the name of the hook (monitored function)
  std::string name = PIN_UndecorateSymbolName(RTN_Name(rtn),
    UNDECORATION_NAME_ONLY);

  // If the function is a hook, it should be in the map
  HookInfoMap::iterator it = m_hooks.find(name);

  if (it != m_hooks.end())
  { // Function is in the map, it is a hook
    if (hi != NULL)
    { // Save the pointer to the information to the location specified by user
      *hi = it->second;
    }

    return true;
  }

  return false; // Function not found in the map, not a hook
}

/**
 * Checks if a function is a noise point.
 *
 * @param rtn An object representing the function.
 * @param desc If specified and not @em NULL, a pointer to a structure
 *   containing the description of the noise will be stored here.
 * @return @em True if the function is a noise point, @em false otherwise.
 */
bool Settings::isNoisePoint(RTN rtn, NoiseSettings** ns)
{
  // Determine the name of the noise point function
  std::string name = PIN_UndecorateSymbolName(RTN_Name(rtn),
    UNDECORATION_NAME_ONLY);

  // If the function is a noise point, it should be in the map
  NoiseSettingsMap::iterator it = m_noisePoints.find(name);

  if (it != m_noisePoints.end())
  { // Function is in the map, it is a noise point
    if (ns != NULL)
    { // Save the pointer to the description to the location specified by user
      *ns = it->second;
    }

    return true;
  }

  return false; // Function not found in the map, not a noise point
}

/**
 * Gets a name of the analysed program.
 *
 * @return The name of the analysed program.
 */
std::string Settings::getProgramName()
{
  return m_program.filename().string();
}

/**
 * Gets a path to the analysed program.
 *
 * @return The path to the analysed program.
 */
std::string Settings::getProgramPath()
{
  return m_program.string();
}

/**
 * Gets a path to a file containing the coverage information.
 *
 * @param type A type of the concurrent coverage.
 * @return The path to the file containing the coverage information.
 */
std::string Settings::getCoverageFile(ConcurrentCoverage type)
{
  fs::path file = m_settings["coverage.directory"].as< fs::path >() /
    expandVars(m_settings["coverage.filename"].as< std::string >(),
      this->getCoverageFilenameVariables(type));

  return file.string();
}

/**
 * Determines a full path to a configuration file given as a relative path. The
 *   method searches various folders in order the locate the configuration file
 *   using its relative path. The following folders are searched (listed in the
 *   order they are searched):
 *
 *     1) The directory specified using the --config option
 *     2) The current directory
 *     3) The ~/.anaconda directory (user-specific configuration)
 *     4) The /etc/anaconda directory (system-wide configuration)
 *
 *   When a configuration file is found, its full path is returned and no other
 *   folders are searches, i.e., the search ends when the configuration file is
 *   found, ignoring similar configurations files in the lower priority folders.
 *
 * @param path A relative path to a configuration file.
 * @return A full path to a configuration file or an empty path if the file is
 *   not found.
 */
fs::path Settings::getConfigFile(fs::path path)
{
  // A list of all directories we need to search through
  std::list< fs::path > directories = boost::assign::list_of
    (m_settings["config"].as< fs::path >())
    (fs::current_path())
    (this->expandEnvVars("${HOME}/.anaconda"))
#ifdef TARGET_WINDOWS
    (this->expandEnvVars("${CYGWIN_HOME}/etc/anaconda"));
#else
    ("/etc/anaconda");
#endif

  BOOST_FOREACH(fs::path dir, directories)
  { // Search each directory, return the first file found
    if (fs::exists(dir / path)) return dir / path;
  }

  // Configuration file not found, return an empty path
  return fs::path();
}

/**
 * Loads the ANaConDA framework's general settings.
 *
 * @param argc A number of arguments passed to the PIN run script.
 * @param argv A list of arguments passed to the PIN run script.
 * @throw SettingsError if the settings contain errors.
 */
void Settings::loadSettings(int argc, char **argv) throw(SettingsError)
{
  // Helper variables
  po::options_description config;
  po::options_description cmdline;
  po::options_description both;

  // Define the options which can be set in the configuration file
  config.add_options()
    ("backtrace.type", po::value< std::string >()->default_value("none"))
    ("backtrace.verbosity", po::value< std::string >()->default_value("detailed"))
    ("coverage.synchronisation", po::value< bool >()->default_value(false))
    ("coverage.sharedvars", po::value< bool >()->default_value(false))
    ("coverage.predecessors", po::value< bool >()->default_value(false))
    ("coverage.filename", po::value< std::string >()->default_value("{ts}-{pn}.{cts}"))
    ("coverage.directory", po::value< fs::path >()->default_value(fs::path("./coverage")))
    ("noise.filters", po::value< std::string >()->default_value(""))
    ("noise.filters.sharedvars.type",
      po::value< std::string >()->default_value("all"))
    ("noise.filters.sharedvars.file",
      po::value< std::string >()->default_value("./coverage/{lts}-{pn}.{cts}"))
    ("noise.filters.predecessors.file",
      po::value< std::string >()->default_value("./coverage/{lts}-{pn}.{cts}"))
    ("noise.type", po::value< std::string >()->default_value("sleep"))
    ("noise.frequency", po::value< int >()->default_value(0))
    ("noise.strength", po::value< int >()->default_value(0));

  // Define the options which can be set through the command line
  cmdline.add_options()
    ("show-settings", po::value< bool >()->default_value(false)->zero_tokens())
    ("show-dbg-info", po::value< bool >()->default_value(false)->zero_tokens())
    ("config,c", po::value< fs::path >()->default_value(fs::current_path()));

  // Define the options which can be set using both the above methods
  both.add_options()
    ("analyser,a", po::value< fs::path >()->default_value(fs::path("")))
    ("debug,d", po::value< std::string >()->default_value("none"))
    ("seed", po::value< UINT64 >());

  // Move the argument pointer to the argument holding the path to the ANaConDA
  // framework's library (path to a .dll file on Windows or .so file on Linux)
  while (std::string(*argv++) != "-t");

  // After the path are the ANaConDA framework's arguments up to the '--' string
  // separating them from the executed binary's arguments, count the arguments
  // and leave the path as the first argument (will be skipped by the parser)
  for (argc = 0; std::string(argv[argc]) != "--"; argc++);

  // Store the path to the analysed program (some parts of the library need it)
  m_program = fs::path(argv[argc + 1]);

  // Store the path to the ANaConDA framework's library (will be needed later)
  m_library = boost::ends_with(argv[0], SHLIB_EXT)
   ? fs::path(FORMAT_STR("%1%", argv[0]))
   : fs::path(FORMAT_STR("%1%%2%", argv[0] % SHLIB_EXT));

  // Load the settings from the command line arguments and store them in a map
  store(parse_command_line(argc, argv, cmdline.add(both)), m_settings);
  notify(m_settings);

  // Get the path to the main configuration file
  fs::path file = this->getConfigFile("anaconda.conf");

  // Check if the configuration file was found
  if (file.empty()) SETTINGS_ERROR("anaconda.conf not found.");

  try
  { // Load the settings from the default or user-specified configuration file
    fs::fstream f(file);

    // Store only settings not specified through the command line in the map
    store(parse_config_file(f, config.add(both), true), m_settings);
    notify(m_settings);

    // Helper macros used only in this method
    #define SPECIAL_CASE_OPTION(name, base, type) \
      (name, po::value< type >()->default_value(m_settings[base].as< type >()))
    #define SPECIAL_CASE_NOISE_OPTION(prefix) \
      SPECIAL_CASE_OPTION(prefix".filters", "noise.filters", std::string) \
      SPECIAL_CASE_OPTION(prefix".filters.sharedvars.type", \
        "noise.filters.sharedvars.type", std::string) \
      SPECIAL_CASE_OPTION(prefix".type", "noise.type", std::string) \
      SPECIAL_CASE_OPTION(prefix".frequency", "noise.frequency", int) \
      SPECIAL_CASE_OPTION(prefix".strength", "noise.strength", int)

    // Special case options use values of other options as their default values,
    // so we need to add them now, when we have all the default values loaded
    config.add_options()
      SPECIAL_CASE_NOISE_OPTION("noise.read")
      SPECIAL_CASE_NOISE_OPTION("noise.write")
      SPECIAL_CASE_NOISE_OPTION("noise.update");

    // Process the configuration file once more (now with special options)
    f.clear();
    f.seekg(0, ios::beg);

    // Store only special case settings not present in the first run above
    store(parse_config_file(f, config), m_settings);
    notify(m_settings);
  }
  catch (std::exception& e)
  { // Error while loading the configuration file, probably contains errors
    SETTINGS_ERROR(FORMAT_STR(
      "could not load settings from the configuration file: %1%", e.what()));
  }

  // Extract the noise injection settings for each type of memory accesses
  m_readNoise = this->loadNoiseSettings("noise.read");
  m_writeNoise = this->loadNoiseSettings("noise.write");
  m_updateNoise = this->loadNoiseSettings("noise.update");
}

/**
 * Loads the noise injection settings for a specific type of memory accesses.
 *
 * @param prefix A string defining a section in the configuration file which
 *   contains the noise injection settings for a specific type of memory
 *   accesses.
 * @return A structure containing noise injection settings for a specific type
 *   of memory accesses.
 * @throw SettingsError if the settings contain errors.
 */
NoiseSettings* Settings::loadNoiseSettings(std::string prefix)
  throw(SettingsError)
{
  // First load the information about the generator and its parameters
  NoiseSettings* ns = new NoiseSettings(
    m_settings[prefix + ".type"].as< std::string >(),
    m_settings[prefix + ".frequency"].as< int >(),
    m_settings[prefix + ".strength"].as< int >());

  // Load filters, filters are stored in a comma-separated list
  boost::tokenizer< boost::char_separator< char > >
    tokenizer(m_settings[prefix + ".filters"].as< std::string >(),
      boost::char_separator< char >(","));

  // Get the filters as a vector
  std::vector< std::string > filters(tokenizer.begin(), tokenizer.end());

  // A list of supported filters
  std::map< std::string, NoiseFilter > supported = boost::assign::map_list_of
    ("sharedvars", NF_SHARED_VARS)
    ("predecessors", NF_PREDECESSORS)
    ("inverse", NF_INVERSE_NOISE);

  BOOST_FOREACH(std::string filter, filters)
  { // For each filter, check if it is supported and add it to noise settings
    boost::trim(filter);

    if (!supported.count(filter))
      SETTINGS_ERROR(FORMAT_STR("unknown filter '%1%'.", filter));

    ns->filters.push_back(supported[filter]);

    switch (ns->filters.back())
    { // Process filter properties
      case NF_SHARED_VARS: // Shared variables filter properties
        filter = m_settings["noise.filters.sharedvars.type"].as< std::string >();

        if (filter != "all" && filter != "one")
        { // Only sharedVars-all and sharedVars-one filters are supported
          SETTINGS_ERROR(FORMAT_STR(
            "unknown shared variables filter type '%1%'.", filter));
        }

        ns->properties.set("svars.type", filter);
        break;
      case NF_PREDECESSORS: // Predecessors filter properties
        break;
      case NF_INVERSE_NOISE: // Inverse noise filter properties
        break;
      default: // Something is very wrong if the control reaches this part
        assert(false);
        break;
    }
  }

  return ns; // Return the loaded noise settings
}

/**
 * Loads values of environment variables.
 */
void Settings::loadEnvVars()
{
  getEnvVars(m_env);
}

/**
 * Loads patterns describing images (executables, shared objects, dynamic
 *   libraries, ...) which should be filtered (included or excluded) from
 *   instrumentation and/or debugging information extraction.
 */
void Settings::loadFilters()
{
  // A directory containing files with filter definitions
  fs::path root("filters");

  // A table mapping filter definitions to their internal representation
  typedef std::map< fs::path, PatternList* > FilterMapping;

  // A list of filter definitions that will be loaded
  FilterMapping filters = boost::assign::map_list_of
    (root / "ins" / "exclude", &m_insExclusions)
    (root / "ins" / "include", &m_insInclusions)
    (root / "die" / "exclude", &m_dieExclusions)
    (root / "die" / "include", &m_dieInclusions);

  BOOST_FOREACH(FilterMapping::value_type filter, filters)
  { // Load all filter definitions from a file and store them internally
    this->loadFiltersFromFile(this->getConfigFile(filter.first), *filter.second);
  }
}

/**
 * Loads patterns describing images (executables, shared objects, dynamic
 *   libraries, ...) which should be filtered (included or excluded) from
 *   instrumentation and/or debugging information extraction from a file.
 */
void Settings::loadFiltersFromFile(fs::path file, PatternList& list)
{
  if (fs::exists(file))
  { // Extract all patterns from a file
    fs::fstream f(file);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Skip all commented and empty lines
      if (line.empty() || line[0] == '#') continue;
      // Normalise the path to a format used by the target operating system
      replace(line.begin(), line.end(), PATH_SEP_CHAR_ALT, PATH_SEP_CHAR);
      // Each line of the file contain one blob pattern
      std::string blob = this->expandEnvVars(line);
      // No function for blob filtering, use regex, but show blob to users
      list.push_back(make_pair(blob, std::regex(this->blobToRegex(blob))));
    }
  }
}

/**
 * Loads all hooks, i.e., definitions of functions which should be monitored by
 *   the framework. When some monitored function is executed, the framework
 *   notifies all the registered listeners about this event.
 */
void Settings::loadHooks()
{
  // A directory containing files with hook definitions
  fs::path root("hooks");

  // A table mapping hook definitions to their type
  typedef std::map< fs::path, HookType > HookMapping;

  // A list of hook definitions that will be loaded
  HookMapping hooks = boost::assign::map_list_of
    (root / "lock", HT_LOCK)
    (root / "unlock", HT_UNLOCK)
    (root / "signal", HT_SIGNAL)
    (root / "wait", HT_WAIT)
    (root / "lock_init", HT_LOCK_INIT)
    (root / "generic_wait", HT_GENERIC_WAIT)
    (root / "thread_create", HT_THREAD_CREATE)
    (root / "thread_init", HT_THREAD_INIT)
    (root / "join", HT_JOIN)
    (root / "tx_start", HT_TX_START)
    (root / "tx_commit", HT_TX_COMMIT)
    (root / "tx_abort", HT_TX_ABORT)
    (root / "tx_read", HT_TX_READ)
    (root / "tx_write", HT_TX_WRITE)
    (root / "noise_point", HT_NOISE_POINT);

  BOOST_FOREACH(HookMapping::value_type hook, hooks)
  { // Load all hook definitions from a file
    this->loadHooksFromFile(this->getConfigFile(hook.first), hook.second);
  }
}

/**
 * Loads hooks of a specific type from a file.
 *
 * @param file A file containing the hooks.
 * @param type A type of hooks contained in the file.
 */
void Settings::loadHooksFromFile(fs::path file, HookType type)
{
  if (!fs::exists(file))
  { // Do not threat non-existent hook files as error, but log the problem
    LOG("Could not load " + std::string(g_hookTypeString[type])
      + "s (hooks): file '" + file.string() + "' not found.\n");
    return;
  }

  // Helper classes
  class HookSpecification
  { // Splits hook specification into separate parts which can be accessed
    private: // Internal variables for storing parts of hook specification
      typedef boost::tokenizer< boost::char_separator< char > > Tokenizer;
      Tokenizer m_parts; // Container for storing hook specification parts
      Tokenizer::iterator m_it; // Pointer to the currently processed part
    public: // Process a hook specification, parts are separated by spaces
      HookSpecification(std::string& l) : m_parts(l, boost::char_separator
        < char >(" ")), m_it(m_parts.begin()) {};
    public: // Methods for checking and accessing hook specification parts
      bool hasMoreParts() { return m_it != m_parts.end(); }
      std::string nextPart() { return *m_it++; }
  };

  // Helper variables
  fs::fstream f(file);
  std::string line;

  while (std::getline(f, line) && !f.fail())
  { // Each line is a hook specification or a comment
    if (line[0] == '#') continue; // Skip comments

    // Line contains a hook specification, process it
    HookSpecification hs(line);

    // Skip empty lines
    if (!hs.hasMoreParts()) continue;

    // First part is always the name of the hook (function to be monitored)
    std::string name = hs.nextPart();

    if (type < HT_TX_START)
    { // Synchronisation function (lock, unlock, signal, wait, ...)
      if (!hs.hasMoreParts())
      { // Incomplete specification, the index part is missing
        LOG("Ignoring incomplete " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': index of the synchronisation primitive is missing.\n");
        continue;
      }

      // Helper variables
      int idx;

      try
      { // The second part is the index of a synchronisation primitive
        idx = boost::lexical_cast< int >(hs.nextPart());
      }
      catch (boost::bad_lexical_cast &)
      { // Invalid specification, index must be a number from [-1, \infinity)
        LOG("Ignoring invalid " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': the index of a synchronisation primitive is not a number.\n");
        continue;
      }

      if (!hs.hasMoreParts())
      { // Incomplete specification, the mapper object part is missing
        LOG("Ignoring invalid " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': the specification of a mapper object is missing.\n");
        continue;
      }

      // Third part is a specification of a mapper object, format: <name>([*]*)
      std::regex re("([a-zA-Z0-9]+)\\(([*]*)\\)");
      std::smatch mo;

      // The match result contains references to the given string, so the string
      // must be valid after the regex_match call, cannot give it a return value
      std::string mopart = hs.nextPart();

      if (!regex_match(mopart, mo, re))
      { // Invalid specification, mapper object must be in a format <name>([*]*)
        LOG("Ignoring invalid " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': the specification of a mapper object is invalid.\n");
        continue;
      }

      if (GET_MAPPER(mo[1].str()) == NULL)
      { // Invalid mapper object, no mapper object of the specified name exist
        LOG("Ignoring invalid " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': unknown mapper object '" + mo[1].str() + "'\n.");
        continue;
      }

      // Valid sync hook in format: <function> <index> <mapper>(<refdepth>)
      m_hooks.insert(HookInfoMap::value_type(name, new HookInfo(type, idx,
        mo[2].str().size(), GET_MAPPER(mo[1].str()))));
    }
    else if (HT_TX_START <= type && type <= HT_TX_ABORT)
    { // Transaction management function (start, commit or abort)
      m_hooks.insert(HookInfoMap::value_type(name, new HookInfo(type)));
    }
    else if (HT_TX_READ <= type && type <= HT_TX_WRITE)
    { // Transactional memory access function (read or write)
      if (!hs.hasMoreParts())
      { // Incomplete specification, the index part is missing
        LOG("Ignoring incomplete " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': index of the memory accessed is missing.\n");
        continue;
      }

      // Actual token is the index of a memory accessed, format: <idx>([*]*)
      std::regex re("([0-9]+)(\\(([*]*)\\))??");
      std::smatch mem;

      // The match result contains references to the given string, so the string
      // must be valid after the regex_match call, cannot give it a return value
      std::string mempart = hs.nextPart();

      if (!regex_match(mempart, mem, re))
      { // Invalid specification, format <idx>([*]*)
        LOG("Ignoring invalid " + std::string(g_hookTypeString[type])
          + " (hook) specification in file '" + file.string()
          + "': the index of the accessed memory address is invalid.\n");
        continue;
      }

      // Valid transactional access in format: <function> <index>(<refdepth>)
      m_hooks.insert(HookInfoMap::value_type(name, new HookInfo(type,
        boost::lexical_cast< unsigned int >(mem[1]), mem[3].str().size())));
    }

    if (hs.hasMoreParts())
    { // Noise settings specified, format: <generator>(frequency,strength)
      std::regex re("([a-zA-Z0-9]+)\\(([0-9]+)[,]([0-9]+)\\)");
      std::smatch ns;

      // The match result contains references to the given string, so the string
      // must be valid after the regex_match call, cannot give it a return value
      std::string nspart = hs.nextPart();

      if (!regex_match(nspart, ns, re))
      { // Ignore this noise point, but continue processing the remaining hooks
        LOG("Ignoring invalid noise settings for hook '" + name + "' in file '"
          + file.string() + "'.\n");
        continue;
      }

      // Valid noise settings, frequency and strength are numbers
      m_noisePoints.insert(NoiseSettingsMap::value_type(name, new NoiseSettings(
          ns[1], boost::lexical_cast< unsigned int >(ns[2]),
          boost::lexical_cast< unsigned int >(ns[3]))));
    }
    else
    { // No noise settings specified for the hook, use the global settings
      m_noisePoints.insert(NoiseSettingsMap::value_type(name, new NoiseSettings(
        m_settings["noise.type"].as< std::string >(),
        m_settings["noise.frequency"].as< int >(),
        m_settings["noise.strength"].as< int >())));
    }
  }
}

/**
 * Loads a program analyser.
 *
 * @throw SettingsError if the settings contain errors.
 */
void Settings::loadAnalyser() throw(SettingsError)
{
  // Check if the analyser's library (path to its .dll or .so file) exists
  if (!fs::exists(m_settings["analyser"].as< fs::path >()))
    SETTINGS_ERROR(FORMAT_STR("analyser's library %1% not found.",
      m_settings["analyser"].as< fs::path >()));

  // Helper variables
  std::string error;

  // Load the ANaConDA framework's library (already loaded by the PIN framework,
  // but this will make the exported symbols accessible to the program analyser)
  m_anaconda = SharedLibrary::Load(m_library, error);

  // Check if the ANaConDA framework's library was loaded successfully
  if (m_anaconda == NULL)
    SETTINGS_ERROR(FORMAT_STR(
      "could not load the ANaConDA framework's library %1%: %2%",
      m_library % error));

  // Load the program analyser (.dll or .so file)
  m_analyser = Analyser::Load(m_settings["analyser"].as< fs::path >(), error);

  // Check if the analyser was loaded successfully
  if (m_analyser == NULL)
    SETTINGS_ERROR(FORMAT_STR(
      "could not load the analyser's library %1%: %2%",
      m_settings["analyser"].as< fs::path >() % error));

#ifdef TARGET_WINDOWS
  // Get the instance of the ANaConDA framework hidden by a custom PIN loader
  SharedLibrary* anaconda = SharedLibrary::Get(ANACONDA_FRAMEWORK);
  SharedLibrary* pin = SharedLibrary::Get(PIN_FRAMEWORK);

  // Redirect all calls from the analyser to the hidden ANaConDA framework or
  // the analyser will not receive any notifications from the framework. This
  // is because the analyser is currently bound to another ANaConDA framework
  // instance (the one loaded above), which is visible to the system. Calling
  // callback registration functions causes the callbacks to be registered in
  // the wrong instance of the ANaConDA framework which PIN ignores and looks
  // only what is registered in the hidden instance of the ANaConDA framework.
  // Therefore, we need to redirect all the registration calls to the hidden
  // instance of the ANaConDA framework in order to get the callbacks working.
  m_analyser->rebind(anaconda);
  // Redirect all calls from the analyser to the hidden PIN framework just to
  // be sure that the analyser uses the same instance of the PIN framework as
  // the ANaConDA framework.
  m_analyser->rebind(pin);

  // This will not free the library as the handle is unknown to the system
  delete anaconda;
  delete pin;
#endif

#ifdef TARGET_LINUX
  // If debugging the analyser, print information needed by the GDB debugger
  if (m_settings["debug"].as< std::string >() == "analyser")
  { // To successfully debug the analyser, GDB needs addresses of few sections
    GElf_Section_Map sections;
    // Get information about all sections in an ELF binary (shared object here)
    gelf_getscns(m_analyser->getLibraryPath().native().c_str(), sections);
    // Get the base address at which was the analyser loaded
    uintptr_t base = (uintptr_t)m_analyser->getLibraryAddress();
    // Print information about the .text, .data and .bss sections needed by GDB
    CONSOLE_NOPREFIX("add-symbol-file " + m_analyser->getLibraryPath().native()
      + " " + hexstr(base + sections[".text"].sh_addr)
      + " -s .data " + hexstr(base + sections[".data"].sh_addr)
      + " -s .bss " + hexstr(base + sections[".bss"].sh_addr)
      + "\n");
  }
  else if (m_settings["debug"].as< std::string >() == "framework")
  { // To successfully debug the framework, GDB needs info about shared objects
    dl_sobj_info_list infos;
    // Get the information about all shared objects loaded by the framework
    dl_get_sobjs(infos);
    // Print information about all shared objects loaded by the framework
    BOOST_FOREACH(dl_sobj_info info, infos)
    { // Do not print information about shared objects without a name
      if (std::string(info.dlsi_name).empty()) continue;
      // Helper variables
      GElf_Section_Map sections;
      // Get all sections of a shared object file
      gelf_getscns(info.dlsi_name, sections);
      // Print information about .text, .data and .bss sections needed by GDB
      CONSOLE_NOPREFIX("add-symbol-file " + std::string(info.dlsi_name)
        + " " + hexstr(info.dlsi_addr + sections[".text"].sh_addr)
        + " -s .data " + hexstr(info.dlsi_addr + sections[".data"].sh_addr)
        + " -s .bss " + hexstr(info.dlsi_addr + sections[".bss"].sh_addr)
        + "\n");
    }
  }
#endif

  // Initialise the analyser (e.g. execute its initialisation code)
  m_analyser->init();
}

/**
 * Setups the ANaConDA framework's noise settings.
 *
 * @throw SettingsError if the noise settings contain errors.
 */
void Settings::setupNoise() throw(SettingsError)
{
  // If seed not specified by the user, use the number of microseconds
  m_seed = m_settings.count("seed") ? m_settings["seed"].as< UINT64 >()
    : m_timestamp.time_of_day().fractional_seconds();

  // Get a function which should inject noise before all reads
  m_readNoise->generator = GET_NOISE_GENERATOR(m_readNoise->gentype);

  if (m_readNoise->generator == NULL)
  { // There is no noise injection function for the specified type
    SETTINGS_ERROR("Unknown noise type '" + m_readNoise->gentype + "'.");
  }

  // Get a function which should inject noise before all writes
  m_writeNoise->generator = GET_NOISE_GENERATOR(m_writeNoise->gentype);

  if (m_writeNoise->generator == NULL)
  { // There is no noise injection function for the specified type
    SETTINGS_ERROR("Unknown noise type '" + m_writeNoise->gentype + "'.");
  }

  // Get a function which should inject noise before all updates
  m_updateNoise->generator = GET_NOISE_GENERATOR(m_updateNoise->gentype);

  if (m_updateNoise->generator == NULL)
  { // There is no noise injection function for the specified type
    SETTINGS_ERROR("Unknown noise type '" + m_updateNoise->gentype + "'.");
  }

  BOOST_FOREACH(NoiseSettingsMap::value_type noise, m_noisePoints)
  { // Get a function which should inject noise before specific function calls
    noise.second->generator = GET_NOISE_GENERATOR(noise.second->gentype);

    if (noise.second->generator == NULL)
    { // There is no noise injection function for the specified type
      SETTINGS_ERROR("Unknown noise type '" + noise.second->gentype + "'.");
    }
  }

  // Helper variables
  std::set< NoiseFilterList::value_type > filters;

  // Merge all the filters to a single set
  filters.insert(m_readNoise->filters.begin(), m_readNoise->filters.end());
  filters.insert(m_writeNoise->filters.begin(), m_writeNoise->filters.end());
  filters.insert(m_updateNoise->filters.begin(), m_updateNoise->filters.end());

  if (filters.count(NF_SHARED_VARS))
  { // Determine path to file containing shared variables (given as pattern)
    VarMap map = this->getCoverageFilenameVariables(CC_SVARS);
    map.insert(VarMap::value_type("lts", pt::to_iso_string(
      this->getLastTimestamp(CC_SVARS))));
    std::string file = expandVars(
      m_settings["noise.filters.sharedvars.file"].as< std::string >(), map);

    if (fs::exists(file))
    { // If the path (expanded pattern) is valid, load the shared variables
      m_coverage.svars.load(file);

      LOG("Shared variables loaded from file '" + file + "'.\n");
    }
    else SETTINGS_ERROR(FORMAT_STR(
      "File '%1%' containing the shared variables not found!\n", file));
  }

  if (filters.count(NF_PREDECESSORS))
  { // Determine path to file containing predecessors (given as pattern)
    VarMap map = this->getCoverageFilenameVariables(CC_PREDS);
    map.insert(VarMap::value_type("lts", pt::to_iso_string(
      this->getLastTimestamp(CC_PREDS))));
    std::string file = expandVars(
      m_settings["noise.filters.predecessors.file"].as< std::string >(), map);

    if (fs::exists(file))
    { // If the path (expanded pattern) is valid, load the predecessors
      m_coverage.preds.load(file);

      LOG("Predecessors loaded from file '" + file + "'.\n");
    }
    else SETTINGS_ERROR(FORMAT_STR(
      "File '%1%' containing the predecessors not found!\n", file));
  }
}

/**
 * Setups the ANaConDA framework's coverage monitoring settings.
 *
 * @throw SettingsError if the coverage monitoring settings contain errors.
 */
void Settings::setupCoverage() throw(SettingsError)
{
  if (m_settings["coverage.synchronisation"].as< bool >())
  { // If synchronisation coverage should be monitored, set the output file
    m_coverage.sync.open(this->getCoverageFile(CC_SYNC));
  }

  if (m_settings["coverage.sharedvars"].as< bool >())
  { // If shared variables should be monitored, set the output file
    m_coverage.svars.open(this->getCoverageFile(CC_SVARS));
  }

  if (m_settings["coverage.predecessors"].as< bool >())
  { // If predecessors should be monitored, set the output file
    m_coverage.preds.open(this->getCoverageFile(CC_PREDS));
  }
}

/**
 * Expands all environment variables in a string.
 *
 * @param s A string containing references to one or more environment variables.
 * @return A string with all environment variables replaced by their values.
 */
std::string Settings::expandEnvVars(std::string s)
{
  // Helper variables
  std::string expanded;
  std::string::iterator it = s.begin();

  while (it != s.end())
  { // Search the whole string for references to environment variables
    if (*it == '$')
    { // An environment variable specification or the '$' character must follow
      if (*++it == '$')
      { // Escaped '$' character, insert it to string and move to the next char
        expanded += *it;
      }
      else if (*it == '{')
      { // Beginning of the specification of a name of an environment variable
        std::string name;

        while (++it != s.end())
        { // Get the name of the environment variable
          if (*it == '}')
          { // Valid name specification
            break;
          }
          else
          { // Part of the name specification
            name += *it;
          }
        }

        if (it == s.end())
        { // Name specification incomplete, for now ignore and keep the text
          return expanded + "${" + name;
        }

        // Helper variables
        EnvVarMap::iterator envIt;

        if ((envIt = m_env.find(name)) != m_env.end())
        { // Referenced existing environment variable, replace it with its value
          expanded += envIt->second;
        }
        else
        { // Referenced non-existent environment variable, cannot expand it
          expanded += "${" + name + "}";
        }
      }
      else
      { // Invalid character after '$', for now ignore and keep the text
        expanded += "$" + *it;
      }
    }
    else
    { // Other characters should just be copied to the expanded string
      expanded += *it;
    }

    it++; // Move to the next character in the string
  }

  return expanded; // Return the string with expanded environment variables
}

/**
 * Converts a blob pattern to a corresponding regular expression pattern.
 *
 * @param blob A blob pattern.
 * @return A regular expression pattern corresponding to the blob pattern.
 */
std::string Settings::blobToRegex(std::string blob)
{
  // Helper variables
  std::string regex;
  std::string special(".[{}()\\*+?|^$");

  for (std::string::iterator it = blob.begin(); it != blob.end(); it++)
  { // Convert blob special characters to regular expression equivalents
    if (*it == '*')
    { // '*' in blob corresponds to '.*' in regular expression
      regex.append(".*");
    }
    else if (*it == '?')
    { // '?' in blob corresponds to '.' in regular expression
      regex.push_back('.');
    }
    else if (special.find(*it) != string::npos)
    { // Special characters must be escaped to preserve their meaning in blob
      regex.push_back('\\');
      regex.push_back(*it);
    }
    else
    { // Other characters are treated the same way
      regex.push_back(*it);
    }
  }

  // The regular expression pattern must match the whole string, not only part
  return "^" + regex + "$";
}

/**
 * Gets a map containing values of special variables which can be used in the
 *   pattern defining the name of the file where the coverage will be written.
 *
 * @param type A type of the concurrent coverage.
 * @return A map containing values of special variables which can be used in the
 *   pattern defining the name of the file where the coverage will be written.
 */
VarMap Settings::getCoverageFilenameVariables(ConcurrentCoverage type)
{
  return boost::assign::map_list_of
    ("pn", this->getProgramName()) // Program Name
    ("ts", pt::to_iso_string(m_timestamp)) // TimeStamp
    ("ct", g_concurrentCoverageString[type]) // Coverage Type
    ("cts", g_concurrentCoverageShortString[type]); // Coverage Type Short
}

/**
 * Gets a timestamp of the last file containing the coverage of a specific type.
 *
 * @note The timestamp is extracted from the name of the file, so if the names
 *   do not contain the timestamps, the last timestamp will not be extracted.
 *
 * @param type A type of the concurrent coverage.
 * @return The timestamp of the last file containing the coverage of the
 *   specified type or the current timestamp if the last timestamp could not be
 *   determined.
 */
pt::ptime Settings::getLastTimestamp(ConcurrentCoverage type)
{
  // Search the names of coverage files produced before for the last timestamp
  std::string format = m_settings["coverage.filename"].as< std::string >();

  // Format must define where the timestamp is or we cannot extract it
  if (format.find("{ts}") == std::string::npos) return m_timestamp;

  // The timestamp is what we are looking for, flag it as a regex group
  boost::algorithm::replace_first(format, "{ts}", "([0-9T.]+)");

  // The coverage files produced before are in this directory
  fs::path dir = m_settings["coverage.directory"].as< fs::path >();

  // OK, we cannot search a non-existing directory
  if (!fs::exists(dir)) return m_timestamp;

  // Default constructor creates past-the-end iterator
  fs::directory_iterator end;

  // Helper variables
  std::smatch result;
  std::regex exp(expandVars(format, this->getCoverageFilenameVariables(type)));
  std::string lts;

  for (fs::directory_iterator it(dir); it != end; ++it)
  { // Help CODAN determine what data type the iterator references
    const fs::directory_entry& entry = *it;

    // Search only the directory (no recursion)
    if (fs::is_regular_file(entry.status()))
    { // We are interested in files only, ignore everything else
      if (regex_match(entry.path().filename().string(), result, exp))
      { // The last timestamp is the one with the latest time
        if (result[1] > lts) lts = result[1];
      }
    }
  }

  // If last timestamp is not found, return the current one, else the last
  return lts.empty() ? m_timestamp : pt::from_iso_string(lts);
}

/** End of file settings.cpp **/
