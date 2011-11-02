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
 * @date      Last Update 2011-11-02
 * @version   0.1.3.3
 */

#include "settings.h"

#ifdef TARGET_LINUX
  #include <boost/tokenizer.hpp>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

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
    case NORMAL: // Normal function
      s << "normal function";
      break;
    case LOCK: // Lock function
      s << "lock function (lock=" << value.lock << ",plvl=" << value.plvl
        << ")";
      break;
    case UNLOCK: // Unlock function
      s << "unlock function (lock=" << value.lock << ",plvl=" << value.plvl
        << ")";
      break;
    default: // Something is very wrong if the code reaches this place
      assert(false);
      break;
  }

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

  // Load patterns describing excluded images
  this->loadExclusions();

  // Load names of functions for thread synchronisation
  this->loadSyncFunctions();
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
  PatternList::iterator pIt;
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
  s << "\nImages which will not be instrumented"
    << "\n-------------------------------------\n";

  for (pIt = m_insExclusions.begin(); pIt != m_insExclusions.end(); pIt++)
  { // Print each instrumentation exclusion pattern
    s << pIt->first << std::endl;
  }

  // Print a section containing loaded debug info extraction exclusion patterns
  s << "\nImages whose debugging information will not be extracted"
    << "\n--------------------------------------------------------\n";

  for (pIt = m_dieExclusions.begin(); pIt != m_dieExclusions.end(); pIt++)
  { // Print each debug info extraction exclusion pattern
    s << pIt->first << std::endl;
  }

  // Print a section containing loaded names of thread synchronisation functions
  s << "\nNames of functions for thread synchronisation"
    << "\n---------------------------------------------\n";

  for (fIt = m_syncFunctions.begin(); fIt != m_syncFunctions.end(); fIt++)
  { // Print each name of a synchronisation function with its description
    s << fIt->first << " [" << fIt->second << "]" << std::endl;
  }
}

/**
 * Checks if an image is excluded from instrumentation.
 *
 * @param image An image.
 * @return @em True if the image is excluded from instrumentation, @em false
 *   otherwise.
 */
bool Settings::isExcludedFromInstrumentation(IMG image)
{
  // Helper variables
  PatternList::iterator it;

  // Extract the name of the image (should be a file name which can be matched)
  std::string name = IMG_Name(image);

  for (it = m_insExclusions.begin(); it != m_insExclusions.end(); it++)
  { // Try to match the file name to any of the exclusion patterns
    if (regex_match(name, it->second)) return true;
  }

  // No pattern matches the file name, the image is not excluded
  return false;
}

/**
 * Checks if an image is excluded from debugging information extraction.
 *
 * @param image An image.
 * @return @em True if the image is excluded from debugging information
 *   extraction, @em false otherwise.
 */
bool Settings::isExcludedFromDebugInfoExtraction(IMG image)
{
  // Helper variables
  PatternList::iterator it;

  // Extract the name of the image (should be a file name which can be matched)
  std::string name = IMG_Name(image);

  for (it = m_dieExclusions.begin(); it != m_dieExclusions.end(); it++)
  { // Try to match the file name to any of the exclusion patterns
    if (regex_match(name, it->second)) return true;
  }

  // No pattern matches the file name, the image is not excluded
  return false;
}

/**
 * Checks if a function is a function for thread synchronisation.
 *
 * @param name A name of the function.
 * @param description A description of the function.
 * @return @em True if the function is a function for thread synchronisation
 *   (and sets the @em description parameter) or @em false is it is a normal
 *   function.
 */
bool Settings::isSyncFunction(RTN rtn, FunctionDesc& description)
{
  // If the function is a sync function, it should be in the map
  FunctionMap::iterator it = m_syncFunctions.find(RTN_Name(rtn));

  if (it != m_syncFunctions.end())
  { // Function in the map, it is a sync function, save its description
    description = it->second;

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
 *   libraries, ...) which should be excluded from instrumentation and/or
 *   debugging information extraction.
 */
void Settings::loadExclusions()
{
  // The framework presumes that configuration files are in the 'conf' directory
  boost::filesystem::path confDir = boost::filesystem::current_path() / "conf";

  // Images excluded from instrumentation are specified in 'ins-excludes' file
  boost::filesystem::path insExcFile = confDir / "ins-excludes";

  if (boost::filesystem::exists(insExcFile))
  { // Extract all instrumentation exclusion patterns
    boost::filesystem::fstream f(insExcFile);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line of the file contain one exclusion pattern
      std::string blob = this->expandEnvVars(line);
      // No function for blob filtering, use regex, but present blob to users
      m_insExclusions.push_back(make_pair(blob, this->blobToRegex(blob)));
    }
  }

  // Images excluded from debug info extraction are specified in 'die-excludes'
  boost::filesystem::path dieExcFile = confDir / "die-excludes";

  if (boost::filesystem::exists(dieExcFile))
  { // Extract all instrumentation exclusion patterns
    boost::filesystem::fstream f(dieExcFile);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line of the file contain one exclusion pattern
      std::string blob = this->expandEnvVars(line);
      // No function for blob filtering, use regex, but present blob to users
      m_dieExclusions.push_back(make_pair(blob, this->blobToRegex(blob)));
    }
  }
}

/**
 * Loads names of functions for thread synchronisation.
 */
void Settings::loadSyncFunctions()
{
  // The framework presumes that configuration files are in the 'conf' directory
  boost::filesystem::path confDir = boost::filesystem::current_path() / "conf";

  // Names of lock functions are specified in the 'lock-functions' file
  boost::filesystem::path lockFncFile = confDir / "lock-functions";

  if (boost::filesystem::exists(lockFncFile))
  { // Extract all names of lock functions
    boost::filesystem::fstream f(lockFncFile);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line contain the description of one lock function
      boost::tokenizer< boost::char_separator< char > >
        tokenizer(line, boost::char_separator< char >(" "));

      // Get the parts of the description as a vector
      std::vector< std::string > tokens(tokenizer.begin(), tokenizer.end());

      // The line must be in the 'name lock plvl' format
      m_syncFunctions.insert(make_pair(tokens[0], FunctionDesc(LOCK,
        boost::lexical_cast< unsigned int >(tokens[1]),
        boost::lexical_cast< unsigned int >(tokens[2]))));
    }
  }

  // Names of lock functions are specified in the 'unlock-functions' file
  boost::filesystem::path unlockFncFile = confDir / "unlock-functions";

  if (boost::filesystem::exists(unlockFncFile))
  { // Extract all names of unlock functions
    boost::filesystem::fstream f(unlockFncFile);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line contain the description of one unlock function
      boost::tokenizer< boost::char_separator< char > >
        tokenizer(line, boost::char_separator< char >(" "));

      // Get the parts of the description as a vector
      std::vector< std::string > tokens(tokenizer.begin(), tokenizer.end());

      // The line must be in the 'name lock plvl' format
      m_syncFunctions.insert(make_pair(tokens[0], FunctionDesc(UNLOCK,
        boost::lexical_cast< unsigned int >(tokens[1]),
        boost::lexical_cast< unsigned int >(tokens[2]))));
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
