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
 * @date      Last Update 2013-03-20
 * @version   0.4.1
 */

#include "settings.h"

#ifdef TARGET_LINUX
  #include "linux/dlutils.h"
  #include "linux/elfutils.h"
#endif

#include <algorithm>

#include <boost/assign/list_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "defs.h"

// Macro definitions
#define PRINT_OPTION(name, type) \
  s << name << " = " << m_settings[name].as< type >() << "\n";
#define PRINT_SETTING(name, value) \
  s << name << " = " << value << "\n";
#define FORMAT_STR(frmt, args) \
  (boost::format(frmt) % args).str()
#define SPECIAL_CASE_OPTION(name, basename, type) \
  (name, po::value< type >()->default_value(m_settings[basename].as< type >()))

#ifdef BOOST_NO_EXCEPTIONS
// Exceptions cannot be used so we must define the throw_exception() manually
namespace boost { void throw_exception(std::exception const& e) { return; } }
#endif

/**
 * @brief An array holding a text description of the types of functions the
 *   framework is able to monitor (a text description of the @c FunctionType
 *   enumeration constants).
 */
const char* g_functionTypeString[] = {
  "normal function",
  "lock function",
  "unlock function",
  "signal function",
  "wait function",
  "lock initialisation function",
  "generic wait function",
  "thread creation function",
  "thread initialisation function",
  "join function"
};

/**
 * @brief An array holding a text description of the concurrent coverage
 *   information the framework can provide (a text description of the
 *   @c ConcurrentCoverage enumeration constants).
 */
const char* g_concurrentCoverageString[] = {
  "none",
  "synchronisation",
  "sharedvars"
};

/**
 * @brief An array holding a short text description of the concurrent coverage
 *   information the framework can provide (a shorter text description of the
 *   @c ConcurrentCoverage enumeration constants).
 */
const char* g_concurrentCoverageShortString[] = {
  "none",
  "sync",
  "svars"
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
 * Prints a function description to a stream.
 *
 * @param s A stream to which the function description should be printed.
 * @param value A function description.
 * @return The stream to which was the function description printed.
 */
std::ostream& operator<<(std::ostream& s, const FunctionDesc& value)
{
  switch (value.type)
  { // Print the function description to the stream based on its type
    case FUNC_NORMAL: // A normal function
      s << "normal function";
      break;
    case FUNC_LOCK: // A lock function
      s << "lock function";
      break;
    case FUNC_UNLOCK: // An unlock function
      s << "unlock function";
      break;
    case FUNC_SIGNAL: // A signal function
      s << "signal function";
      break;
    case FUNC_WAIT: // A wait function
      s << "wait function";
      break;
    case FUNC_LOCK_INIT: // A lock initialisation function
      s << "lock initialisation function";
      break;
    case FUNC_GENERIC_WAIT: // A generic wait function
      s << "generic wait function";
      break;
    case FUNC_THREAD_CREATE: // A thread creation function
      s << "thread creation function";
      break;
    case FUNC_THREAD_INIT: // A thread initialisation function
      s << "thread initialisation function";
      break;
    case FUNC_JOIN: // A join function
      s << "join function";
      break;
    default: // Something is very wrong if the code reaches this place
      assert(false);
      break;
  }

  // Other parts of the function description are the same for all types
  s << "(lock=" << value.lock << ",plvl=" << value.plvl << ",farg=" << hex
    << value.farg << dec << ")";

  // Return the stream to which was the function description printed
  return s;
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
std::string operator+(const std::string& s, const FunctionType& type)
{
  return s + g_functionTypeString[type];
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
std::string operator+(const FunctionType& type, const std::string& s)
{
  return g_functionTypeString[type] + s;
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
std::string operator+(const char* s, const FunctionType& type)
{
  return std::string(s) + g_functionTypeString[type];
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
std::string operator+(const FunctionType& type, const char* s)
{
  return std::string(g_functionTypeString[type]) + s;
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
  // Load general settings
  this->loadSettings(argc, argv);

  // Load environment variables (might be referenced later)
  this->loadEnvVars();

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
  FunctionMap::iterator fIt;
  NoiseMap::iterator nIt;

  // Print the ANaConDA framework settings
  s << "Settings\n"
    << "--------\n";

  // Print a section containing loaded general settings
  s << "\nGeneral settings"
    << "\n----------------\n";

  PRINT_OPTION("config", fs::path);
  PRINT_OPTION("analyser", fs::path);
  PRINT_OPTION("debug", std::string);
  PRINT_OPTION("backtrace.type", std::string);
  PRINT_OPTION("backtrace.verbosity", std::string);
  PRINT_OPTION("coverage.synchronisation", bool);
  PRINT_OPTION("coverage.sharedvars", bool);
  PRINT_OPTION("coverage.filename", std::string);
  PRINT_OPTION("coverage.directory", fs::path);
  PRINT_OPTION("noise.sharedvars", bool);
  PRINT_OPTION("noise.sharedvars.file", std::string);
  PRINT_OPTION("noise.type", std::string);
  PRINT_OPTION("noise.frequency", int);
  PRINT_OPTION("noise.strength", int);
  PRINT_OPTION("noise.read.sharedvars", bool);
  PRINT_OPTION("noise.read.type", std::string);
  PRINT_OPTION("noise.read.frequency", int);
  PRINT_OPTION("noise.read.strength", int);
  PRINT_OPTION("noise.write.sharedvars", bool);
  PRINT_OPTION("noise.write.type", std::string);
  PRINT_OPTION("noise.write.frequency", int);
  PRINT_OPTION("noise.write.strength", int);
  PRINT_OPTION("noise.update.sharedvars", bool);
  PRINT_OPTION("noise.update.type", std::string);
  PRINT_OPTION("noise.update.frequency", int);
  PRINT_OPTION("noise.update.strength", int);

  // Print a section containing internal settings
  s << "\nInternal settings"
    << "\n-----------------\n";

  PRINT_SETTING("timestamp", pt::to_iso_string(m_timestamp));

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

  // Print a section containing loaded names of synchronisation functions
  s << "\nNames of synchronisation functions"
    << "\n----------------------------------\n";

  for (fIt = m_syncFunctions.begin(); fIt != m_syncFunctions.end(); fIt++)
  { // Print the names of synchronisation functions with their description
    s << fIt->first << " [" << *fIt->second;

    if ((nIt = m_noisePoints.find(fIt->first)) != m_noisePoints.end())
    { // The synchronisation function is also a noise point
      s << ",noise point(noise=" << *nIt->second << ")";
    }

    s << "]" << std::endl;
  }

  // Print a section containing loaded names of noise points
  s << "\nNames of noise points"
    << "\n---------------------\n";

  for (nIt = m_noisePoints.begin(); nIt != m_noisePoints.end(); nIt++)
  { // Print the names of noise points with the description of the noise
    s << nIt->first << " [" << *nIt->second << "]" << std::endl;
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
 * Checks if a function is a function for thread synchronisation.
 *
 * @param rtn An object representing the function.
 * @param desc If specified and not @em NULL, a pointer to a structure
 *   containing the description of the function will be stored here.
 * @return @em True if the function is a function for thread synchronisation
 *   or @em false is it is a normal function.
 */
bool Settings::isSyncFunction(RTN rtn, FunctionDesc** desc)
{
  // If the function is a sync function, it should be in the map
  FunctionMap::iterator it = m_syncFunctions.find(RTN_Name(rtn));

  if (it != m_syncFunctions.end())
  { // Function is in the map, it is a function for thread synchronisation
    if (desc != NULL)
    { // Save the pointer to the description to the location specified by user
      *desc = it->second;
    }

    return true;
  }

  return false; // Function not found in the map, must be a normal function
}

/**
 * Checks if a function is a noise point.
 *
 * @param rtn An object representing the function.
 * @param desc If specified and not @em NULL, a pointer to a structure
 *   containing the description of the noise will be stored here.
 * @return @em True if the function is a noise point, @em false otherwise.
 */
bool Settings::isNoisePoint(RTN rtn, NoiseDesc** desc)
{
  // If the function is a noise point, it should be in the map
  NoiseMap::iterator it = m_noisePoints.find(RTN_Name(rtn));

  if (it != m_noisePoints.end())
  { // Function is in the map, it is a noise point
    if (desc != NULL)
    { // Save the pointer to the description to the location specified by user
      *desc = it->second;
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
 * Loads the ANaConDA framework's general settings.
 *
 * @param argc A number of arguments passed to the PIN run script.
 * @param argv A list of arguments passed to the PIN run script.
 * @throw SettingsError if the settings contain errors.
 */
void Settings::loadSettings(int argc, char **argv) throw(SettingsError)
{
  // The framework presumes that settings are in the 'conf/anaconda.conf' file
  fs::path file = fs::current_path() / "conf" / "anaconda.conf";

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
    ("coverage.filename", po::value< std::string >()->default_value("{ts}-{pn}.{cts}"))
    ("coverage.directory", po::value< fs::path >()->default_value(fs::path("./coverage")))
    ("noise.sharedvars", po::value< bool >()->default_value(false))
    ("noise.sharedvars.file", po::value< std::string >()->default_value("./coverage/{lts}-{pn}.{cts}"))
    ("noise.type", po::value< std::string >()->default_value("sleep"))
    ("noise.frequency", po::value< int >()->default_value(0))
    ("noise.strength", po::value< int >()->default_value(0));

  // Define the options which can be set through the command line
  cmdline.add_options()
    ("show-settings", po::value< bool >()->default_value(false)->zero_tokens())
    ("show-dbg-info", po::value< bool >()->default_value(false)->zero_tokens())
    ("config,c", po::value< fs::path >()->default_value(file));

  // Define the options which can be set using both the above methods
  both.add_options()
    ("analyser,a", po::value< fs::path >()->default_value(fs::path("")))
    ("debug,d", po::value< std::string >()->default_value("none"));

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
  m_library = fs::path(FORMAT_STR("%1%%2%", argv[0] % SHLIB_EXT));

  // Load the settings from the command line arguments and store them in a map
  store(parse_command_line(argc, argv, cmdline.add(both)), m_settings);
  notify(m_settings);

  // Check if the path represents a valid configuration file (is required)
  if (!fs::exists(m_settings["config"].as< fs::path >()))
    throw SettingsError(FORMAT_STR("configuration file %1% not found.",
      m_settings["config"].as< fs::path >()));

  try
  { // Load the settings from the default or user-specified configuration file
    fs::fstream f(m_settings["config"].as< fs::path >());

    // Store only settings not specified through the command line in the map
    store(parse_config_file(f, config.add(both), true), m_settings);
    notify(m_settings);

    // Special case options use values of other options as their default values,
    // so we need to add them now, when we have all the default values loaded
    config.add_options()
      SPECIAL_CASE_OPTION("noise.read.sharedvars", "noise.sharedvars", bool)
      SPECIAL_CASE_OPTION("noise.read.type", "noise.type", std::string)
      SPECIAL_CASE_OPTION("noise.read.frequency", "noise.frequency", int)
      SPECIAL_CASE_OPTION("noise.read.strength", "noise.strength", int)
      SPECIAL_CASE_OPTION("noise.write.sharedvars", "noise.sharedvars", bool)
      SPECIAL_CASE_OPTION("noise.write.type", "noise.type", std::string)
      SPECIAL_CASE_OPTION("noise.write.frequency", "noise.frequency", int)
      SPECIAL_CASE_OPTION("noise.write.strength", "noise.strength", int)
      SPECIAL_CASE_OPTION("noise.update.sharedvars", "noise.sharedvars", bool)
      SPECIAL_CASE_OPTION("noise.update.type", "noise.type", std::string)
      SPECIAL_CASE_OPTION("noise.update.frequency", "noise.frequency", int)
      SPECIAL_CASE_OPTION("noise.update.strength", "noise.strength", int);

    // Process the configuration file once more (now with special options)
    f.clear();
    f.seekg(0, ios::beg);

    // Store only special case settings not present in the first run above
    store(parse_config_file(f, config), m_settings);
    notify(m_settings);
  }
  catch (std::exception& e)
  { // Error while loading the configuration file, probably contains errors
    throw SettingsError(FORMAT_STR(
      "could not load settings from the configuration file: %1%", e.what()));
  }

  // Transform the noise settings to noise description objects
  m_readNoise = new NoiseDesc(
    m_settings["noise.read.type"].as< std::string >(),
    m_settings["noise.read.frequency"].as< int >(),
    m_settings["noise.read.strength"].as< int >());
  m_writeNoise = new NoiseDesc(
    m_settings["noise.write.type"].as< std::string >(),
    m_settings["noise.write.frequency"].as< int >(),
    m_settings["noise.write.strength"].as< int >());
  m_updateNoise = new NoiseDesc(
    m_settings["noise.update.type"].as< std::string >(),
    m_settings["noise.update.frequency"].as< int >(),
    m_settings["noise.update.strength"].as< int >());
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
  // The framework presumes that filters are in the 'conf/filters' directory
  fs::path filters = fs::current_path() / "conf" / "filters";

  // Images excluded from instrumentation are specified in 'ins/exclude' file
  this->loadFiltersFromFile(filters / "ins" / "exclude", m_insExclusions);

  // Images included for instrumentation are specified in 'ins/include' file
  this->loadFiltersFromFile(filters / "ins" / "include", m_insInclusions);

  // Images excluded from info extraction are specified in 'die/exclude' file
  this->loadFiltersFromFile(filters / "die" / "exclude", m_dieExclusions);

  // Images included for info extraction are specified in 'die/include' file
  this->loadFiltersFromFile(filters / "die" / "include", m_dieInclusions);
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
      list.push_back(make_pair(blob, boost::regex(this->blobToRegex(blob))));
    }
  }
}

/**
 * Loads names of functions acting as hooks, i.e., invoking notifications when
 *   executed.
 */
void Settings::loadHooks()
{
  // The framework presumes that hooks are in the 'conf/hooks' directory
  fs::path hooks = fs::current_path() / "conf" / "hooks";

  // Names of lock functions are specified in the 'lock' file
  this->loadHooksFromFile(hooks / "lock", FUNC_LOCK);

  // Names of unlock functions are specified in the 'unlock' file
  this->loadHooksFromFile(hooks / "unlock", FUNC_UNLOCK);

  // Names of signal functions are specified in the 'signal' file
  this->loadHooksFromFile(hooks / "signal", FUNC_SIGNAL);

  // Names of wait functions are specified in the 'wait' file
  this->loadHooksFromFile(hooks / "wait", FUNC_WAIT);

  // Names of lock init functions are specified in the 'lock_init' file
  this->loadHooksFromFile(hooks / "lock_init", FUNC_LOCK_INIT);

  // Names of generic wait functions are specified in the 'generic_wait' file
  this->loadHooksFromFile(hooks / "generic_wait", FUNC_GENERIC_WAIT);

  // Names of thread creation functions are specified in the 'thread_create' file
  this->loadHooksFromFile(hooks / "thread_create", FUNC_THREAD_CREATE);

  // Names of thread init functions are specified in the 'thread_init' file
  this->loadHooksFromFile(hooks / "thread_init", FUNC_THREAD_INIT);

  // Names of join functions are specified in the 'join' file
  this->loadHooksFromFile(hooks / "join", FUNC_JOIN);
}

/**
 * Loads names of functions acting as hooks, i.e., invoking notifications when
 *   executed, from a file.
 *
 * @param file A file containing names of the functions.
 * @param type A type of the functions contained in the file.
 */
void Settings::loadHooksFromFile(fs::path file, FunctionType type)
{
  if (fs::exists(file))
  { // Extract all names of the functions
    fs::fstream f(file);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line contains the description of one function or comment
      if (line[0] == '#') continue; // Skip comments

      // Line is a function description, parts are separated by spaces
      boost::tokenizer< boost::char_separator< char > >
        tokenizer(line, boost::char_separator< char >(" "));

      // Get the parts of the description as a vector
      std::vector< std::string > tokens(tokenizer.begin(), tokenizer.end());

      // GCC knows that the token is string, but CODAN cannot evaluate it :S
      #define TOKEN(number) std::string(tokens[number])

      // Definitions of mapper functions are in the '<name>([*]*)' format
      boost::regex re("([a-zA-Z0-9]+)\\(([*]*)\\)");
      // Get parts of function definition as strings
      boost::smatch funcdef;

      if (!regex_match(TOKEN(2), funcdef, re))
      { // The definition of the mapper function is invalid
        LOG("Invalid function specification '" + TOKEN(2) + "' in file '"
          + file.string() + "'.\n");
        continue;
      }

      // Noise definition is optional, check if specified
      if (tokens.size() > 3)
      { // Definitions of noise are in the '<type>(frequency,strength)' format
        re.assign("([a-zA-Z0-9]+)\\(([0-9]+)[,]([0-9]+)\\)");
        // Get the parts of noise definition as strings
        boost::smatch noisedef;

        if (!regex_match(TOKEN(3), noisedef, re))
        { // The definition of the noise is invalid
          LOG("Invalid noise specification '" + TOKEN(3) + "' in file '"
            + file.string() + "'.\n");
          continue;
        }

        // Noise specified and valid, extract frequency and strength
        m_noisePoints.insert(make_pair(TOKEN(0), new NoiseDesc(noisedef[1],
          boost::lexical_cast< unsigned int >(noisedef[2]),
          boost::lexical_cast< unsigned int >(noisedef[3]))));
      }
      else
      { // If no noise is specified for the function, use the global settings
        m_noisePoints.insert(make_pair(TOKEN(0), new NoiseDesc(
          m_settings["noise.type"].as< std::string >(),
          m_settings["noise.frequency"].as< int >(),
          m_settings["noise.strength"].as< int >())));
      }

      // The line must be in the 'name arg funcdef(plvl) [noisedef]' format
      m_syncFunctions.insert(make_pair(TOKEN(0), new FunctionDesc(type,
        boost::lexical_cast< unsigned int >(TOKEN(1)), funcdef[2].str().size(),
        GET_MAPPER(funcdef[1].str()))));
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
    throw SettingsError(FORMAT_STR("analyser's library %1% not found.",
      m_settings["analyser"].as< fs::path >()));

  // Helper variables
  std::string error;

  // Load the ANaConDA framework's library (already loaded by the PIN framework,
  // but this will make the exported symbols accessible to the program analyser)
  m_anaconda = SharedLibrary::Load(m_library, error);

  // Check if the ANaConDA framework's library was loaded successfully
  if (m_anaconda == NULL)
    throw SettingsError(FORMAT_STR(
      "could not load the ANaConDA framework's library %1%: %2%",
      m_library % error));

  // Load the program analyser (.dll or .so file)
  m_analyser = Analyser::Load(m_settings["analyser"].as< fs::path >(), error);

  // Check if the analyser was loaded successfully
  if (m_analyser == NULL)
    throw SettingsError(FORMAT_STR(
      "could not load the analyser's library %1%: %2%",
      m_settings["analyser"].as< fs::path >() % error));

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
  // Get a function which should inject noise before all reads
  m_readNoise->function = GET_NOISE_FUNCTION(m_readNoise->type);

  if (m_readNoise->function == NULL)
  { // There is no noise injection function for the specified type
    throw SettingsError("Unknown noise type '" + m_readNoise->type + "'.");
  }

  // Get a function which should inject noise before all writes
  m_writeNoise->function = GET_NOISE_FUNCTION(m_writeNoise->type);

  if (m_writeNoise->function == NULL)
  { // There is no noise injection function for the specified type
    throw SettingsError("Unknown noise type '" + m_writeNoise->type + "'.");
  }

  // Get a function which should inject noise before all updates
  m_updateNoise->function = GET_NOISE_FUNCTION(m_updateNoise->type);

  if (m_updateNoise->function == NULL)
  { // There is no noise injection function for the specified type
    throw SettingsError("Unknown noise type '" + m_updateNoise->type + "'.");
  }

  BOOST_FOREACH(NoiseMap::value_type noise, m_noisePoints)
  { // Get a function which should inject noise before specific function calls
    noise.second->function = GET_NOISE_FUNCTION(noise.second->type);

    if (noise.second->function == NULL)
    { // There is no noise injection function for the specified type
      throw SettingsError("Unknown noise type '" + noise.second->type + "'.");
    }
  }

  if (m_settings["noise.read.sharedvars"].as< bool >()
   || m_settings["noise.write.sharedvars"].as< bool >()
   || m_settings["noise.update.sharedvars"].as< bool >())
  { // Determine path to file containing shared variables (given as pattern)
    VarMap map = this->getCoverageFilenameVariables(CC_SVARS);
    map.insert(VarMap::value_type("lts", pt::to_iso_string(
      this->getLastTimestamp(CC_SVARS))));
    std::string file = expandVars(
      m_settings["noise.sharedvars.file"].as< std::string >(), map);

    if (fs::exists(file))
    { // If the path (expanded pattern) is valid, load the shared variables
      m_coverage.svars.load(file);
      LOG("Shared variables loaded from file '" + file + "'.\n");
    }
    else throw SettingsError(FORMAT_STR(
      "File '%1%' containing the shared variables not found!\n", file));
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
  boost::smatch result;
  boost::regex exp(expandVars(format, this->getCoverageFilenameVariables(type)));
  std::string lts;

  for (fs::directory_iterator it(dir); it != end; ++it)
  { // Search only the directory (no recursion)
    if (fs::is_regular_file(it->status()))
    { // We are interested in files only, ignore everything else
      if (boost::regex_match(it->path().filename().string(), result, exp))
      { // The last timestamp is the one with the latest time
        if (result[1] > lts) lts = result[1];
      }
    }
  }

  // Convert the last timestamp from text to time
  return pt::from_iso_string(lts);
}

/** End of file settings.cpp **/
