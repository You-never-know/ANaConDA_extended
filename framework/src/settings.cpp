/**
 * @brief A file containing implementation of classes and function for handling
 *   settings.
 *
 * A file containing implementation of classes and function for handling
 *   settings.
 *
 * @file      settings.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-20
 * @date      Last Update 2011-11-28
 * @version   0.1.8.2
 */

#include "settings.h"

#include <map>

#ifdef TARGET_LINUX
  #include <boost/tokenizer.hpp>
#endif

#include <boost/assign/list_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

namespace
{ // Static global variables (usable only within this module)
  std::map< string, NoiseType >
    g_noiseTypeMap = boost::assign::map_list_of
      ("sleep", NOISE_SLEEP)
      ("yield", NOISE_YIELD);
}

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
 * Prints a noise description to a stream.
 *
 * @param s A stream to which the noise description should be printed.
 * @param value A noise description.
 * @return The stream to which was the noise description printed.
 */
std::ostream& operator<<(std::ostream& s, const NoiseDesc& value)
{
  switch (value.type)
  { // Print the noise description to the stream based on its type
    case NOISE_SLEEP: // A noise inserting sleeps before a function
      s << "sleep";
      break;
    case NOISE_YIELD: // A noise inserting yields before a function
      s << "yield";
      break;
    default: // Something is very wrong if the code reaches this place
      assert(false);
      break;
  }

  // The parameters of the noise are always the same, only type may differ
  s << "(" << value.frequency << "," << value.strength << ")";

  // Return the stream to which was the noise description printed
  return s;
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
    << value.farg << dec << ",noise=" << value.noise << ")";

  // Return the stream to which was the function description printed
  return s;
}

/**
 * Loads the ANaConDA framework settings.
 */
void Settings::load()
{
  // Load environment variables (might be referenced later)
  this->loadEnvVars();

  // Load patterns describing included and excluded images
  this->loadFilters();

  // Load names of functions acting as hooks in the program
  this->loadHooks();
}

/**
 * Prints the ANaConDA framework settings.
 *
 * @param s A stream to which should be the settings printed. If no stream is
 *   specified, standard output stream will be used.
 */
void Settings::print(std::ostream& s)
{
  // Helper variables
  EnvVarMap::iterator envIt;
  FunctionMap::iterator fIt;

  // Print the ANaConDA framework settings
  s << "Settings\n"
    << "--------\n";

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

  // Print a section containing loaded names of thread synchronisation functions
  s << "\nNames of functions for thread synchronisation"
    << "\n---------------------------------------------\n";

  for (fIt = m_syncFunctions.begin(); fIt != m_syncFunctions.end(); fIt++)
  { // Print each name of a synchronisation function with its description
    s << fIt->first << " [" << *fIt->second << "]" << std::endl;
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

      // If no noise is specified for the function, do not insert noise
      NoiseDesc noise;

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

        // Translate the string form of a noise type to the corresponding enum
        std::map< std::string, NoiseType >::iterator it = g_noiseTypeMap.find(
          noisedef[1]);

        if (it == g_noiseTypeMap.end())
        { // The type of the noise is invalid
          LOG("Invalid noise type '" + noisedef[1] + "' in file '"
            + file.native() + "'.\n");
          continue;
        }

        // Noise specified and valid, extract frequency and strength
        noise.type = it->second;
        noise.frequency = boost::lexical_cast< unsigned int >(noisedef[2]);
        noise.strength = boost::lexical_cast< unsigned int >(noisedef[3]);
      }

      // The line must be in the 'name arg funcdef(plvl) [noisedef]' format
      m_syncFunctions.insert(make_pair(tokens[0], new FunctionDesc(type,
        boost::lexical_cast< unsigned int >(tokens[1]), funcdef[2].str().size(),
        GET_MAPPER(funcdef[1].str()), noise)));
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
    else
    { // Other characters are treated the same way
      regex += *it;
    }
  }

  // The regular expression pattern must match the whole string, not only part
  return "^" + regex + "$";
}

/** End of file settings.cpp **/
