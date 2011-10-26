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
 * @date      Last Update 2011-10-26
 * @version   0.1.2.1
 */

#include "settings.h"

#ifdef TARGET_LINUX
  #include <boost/tokenizer.hpp>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/**
 * Loads the ANaConDA framework settings.
 */
void Settings::load()
{
  // Load environment variables (might be referenced later)
  this->loadEnvVars();

  // Load patterns describing excluded images
  this->loadExclusions();
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
    s << *pIt << std::endl;
  }
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
  boost::filesystem::path confDir = boost::filesystem::current_path() /= "conf";

  // Images excluded from instrumentation are specified in 'ins-excludes' file
  boost::filesystem::path insExcFile = confDir /= "ins-excludes";

  if (boost::filesystem::exists(insExcFile))
  { // Extract all instrumentation exclusion patterns
    boost::filesystem::fstream f(insExcFile);

    // Helper variables
    std::string line;

    while (std::getline(f, line) && !f.fail())
    { // Each line of the file contain one exclusion pattern
      m_insExclusions.push_back(this->expandEnvVars(line));
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
