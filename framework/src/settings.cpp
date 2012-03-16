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
 * @date      Last Update 2012-03-16
 * @version   0.2.1
 */

#include "settings.h"

#ifdef TARGET_LINUX
  #include <boost/tokenizer.hpp>

  #include "linux/dlutils.h"
  #include "linux/elfutils.h"
#endif

#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// Macro definitions
#define PRINT_OPTION(name, type) \
  s << name << " = " << m_settings[name].as< type >() << "\n";
#define FORMAT_STR(frmt, args) \
  (boost::format(frmt) % args).str()
#define SPECIAL_CASE_OPTION(name, basename, type) \
  (name, po::value< type >()->default_value(m_settings[basename].as< type >()))

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
    case FUNC_NORMAL: // Normal function
      s << "normal function";
      break;
    case FUNC_LOCK: // Lock function
      s << "lock function";
      break;
    case FUNC_UNLOCK: // Unlock function
      s << "unlock function";
      break;
    case FUNC_SIGNAL:
      s << "signal function";
      break;
    case FUNC_WAIT:
      s << "wait function";
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
 * Setups the ANaConDA framework's settings.
 *
 * @throw SettingsError if the settings contain errors.
 */
void Settings::setup() throw(SettingsError)
{
  // Setup the noise settings
  this->setupNoise();
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
  PRINT_OPTION("noise.type", std::string);
  PRINT_OPTION("noise.frequency", int);
  PRINT_OPTION("noise.strength", int);
  PRINT_OPTION("noise.read.type", std::string);
  PRINT_OPTION("noise.read.frequency", int);
  PRINT_OPTION("noise.read.strength", int);
  PRINT_OPTION("noise.write.type", std::string);
  PRINT_OPTION("noise.write.frequency", int);
  PRINT_OPTION("noise.write.strength", int);

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
    ("noise.type", po::value< std::string >()->default_value("sleep"))
    ("noise.frequency", po::value< int >()->default_value(0))
    ("noise.strength", po::value< int >()->default_value(0));

  // Define the options which can be set through the command line
  cmdline.add_options()
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
      SPECIAL_CASE_OPTION("noise.read.type", "noise.type", std::string)
      SPECIAL_CASE_OPTION("noise.read.frequency", "noise.frequency", int)
      SPECIAL_CASE_OPTION("noise.read.strength", "noise.strength", int)
      SPECIAL_CASE_OPTION("noise.write.type", "noise.type", std::string)
      SPECIAL_CASE_OPTION("noise.write.frequency", "noise.frequency", int)
      SPECIAL_CASE_OPTION("noise.write.strength", "noise.strength", int);

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
}

/**
 * Loads values of environment variables.
 */
void Settings::loadEnvVars()
{
#ifdef TARGET_LINUX
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
    m_env[name] = getenv(name.c_str());
  }
#endif
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
      // Each line of the file contain one blob pattern
      std::string blob = this->expandEnvVars(line);
      // No function for blob filtering, use regex, but show blob to users
      list.push_back(make_pair(blob, this->blobToRegex(blob)));
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
    { // Each line contain the description of one function
      boost::tokenizer< boost::char_separator< char > >
        tokenizer(line, boost::char_separator< char >(" "));

      // Get the parts of the description as a vector
      std::vector< std::string > tokens(tokenizer.begin(), tokenizer.end());

      // Definitions of mapper functions are in the '<name>([*]*)' format
      boost::regex re("([a-zA-Z0-9]+)\\(([*]*)\\)");
      // Get parts of function definition as strings
      boost::smatch funcdef;

      if (!regex_match(tokens[2], funcdef, re))
      { // The definition of the mapper function is invalid
        LOG("Invalid function specification '" + tokens[2] + "' in file '"
          + file.native() + "'.\n");
        continue;
      }

      // Noise definition is optional, check if specified
      if (tokens.size() > 3)
      { // Definitions of noise are in the '<type>(frequency,strength)' format
        re.assign("([a-zA-Z0-9]+)\\(([0-9]+)[,]([0-9]+)\\)");
        // Get the parts of noise definition as strings
        boost::smatch noisedef;

        if (!regex_match(tokens[3], noisedef, re))
        { // The definition of the noise is invalid
          LOG("Invalid noise specification '" + tokens[3] + "' in file '"
            + file.native() + "'.\n");
          continue;
        }

        // Noise specified and valid, extract frequency and strength
        m_noisePoints.insert(make_pair(tokens[0], new NoiseDesc(noisedef[1],
          boost::lexical_cast< unsigned int >(noisedef[2]),
          boost::lexical_cast< unsigned int >(noisedef[3]))));
      }
      else
      { // If no noise is specified for the function, use the global settings
        m_noisePoints.insert(make_pair(tokens[0], new NoiseDesc(
          m_settings["noise.type"].as< std::string >(),
          m_settings["noise.frequency"].as< int >(),
          m_settings["noise.strength"].as< int >())));
      }

      // The line must be in the 'name arg funcdef(plvl) [noisedef]' format
      m_syncFunctions.insert(make_pair(tokens[0], new FunctionDesc(type,
        boost::lexical_cast< unsigned int >(tokens[1]), funcdef[2].str().size(),
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

  BOOST_FOREACH(NoiseMap::value_type noise, m_noisePoints)
  { // Get a function which should inject noise before specific function calls
    noise.second->function = GET_NOISE_FUNCTION(noise.second->type);

    if (noise.second->function == NULL)
    { // There is no noise injection function for the specified type
      throw SettingsError("Unknown noise type '" + noise.second->type + "'.");
    }
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
      regex += ".*";
    }
    else if (*it == '?')
    { // '?' in blob corresponds to '.' in regular expression
      regex += ".";
    }
    else if (special.find(*it) != string::npos)
    { // Special characters must be escaped to preserve their meaning in blob
      regex += "\\" + *it;
    }
    else
    { // Other characters are treated the same way
      regex += *it;
    }
  }

  // The regular expression pattern must match the whole string, not only part
  return "^" + regex + "$";
}

/** End of file settings.cpp **/
