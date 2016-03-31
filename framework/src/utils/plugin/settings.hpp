/**
 * @brief Contains a class for handling plugin's settings.
 *
 * A file containing a class for handling plugin's settings.
 *
 * @file      settings.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-03-30
 * @date      Last Update 2016-03-31
 * @version   0.2
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__PLUGIN__SETTINGS_HPP__
  #define __ANACONDA_FRAMEWORK__UTILS__PLUGIN__SETTINGS_HPP__

#include <string>

#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

#include "../../anaconda.h"

// A helper macro simplifying the definition of plugin options
#define OPTION(name, type, defaultvalue) \
  (name, po::value< type >()->default_value(defaultvalue))
// A helper macro simplifying loading of plugin's settings
#define LOAD_SETTINGS(settings, filename) \
  try \
  { \
    settings.load(filename); \
  } \
  catch (std::exception& e) \
  { \
    CONSOLE_NOPREFIX(std::string("warning: could not load settings from file " \
      filename": ") + e.what() + "\n"); \
  }

// Namespace aliases
namespace fs = boost::filesystem;
namespace po = boost::program_options;

namespace boost
{
  namespace program_options
  {
    /**
     * @brief A class thrown when a configuration file is not found.
     */
    class BOOST_PROGRAM_OPTIONS_DECL file_not_found : public error
    {
      public: // Constructors
        /**
         * Constructs a notification that a configuration file was not found.
         *
         * @param filename A name of the configuration file.
         */
        file_not_found(const char* filename)
          : error(std::string("file ").append(filename).append(" not found")) {}
    };
  }
}

/**
 * @brief A class holding the plugin's settings.
 *
 * Holds the plugin's settings.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-03-31
 * @date      Last Update 2016-03-31
 * @version   0.1
 */
class Settings
{
  private: // Internal data
    po::options_description m_options; //!< A list of supported plugin options.
    po::variables_map m_settings; //!< A map containing the plugin's settings.
  public: // Methods for loading, defining and accessing plugin's settings
    /**
     * Loads settings from a configuration file.
     *
     * @param filename A name of the configuration file.
     * @throws @c po::error when the settings cannot be loaded.
     */
    void load(const std::string& filename)
    {
      // Try to locate the configuration file using the ANaConDA framework
      std::string path = SETTINGS_GetConfigFile(filename);

      if (path.empty()) throw po::file_not_found(filename.c_str());

      fs::fstream f(path);

      // Process the configuration file, throws exception on error
      store(parse_config_file(f, m_options), m_settings);
      notify(m_settings);
    }

    /**
     * Gets an object simplifying definition of plugin's options.
     *
     * @return An object simplifying definition of plugin's options.
     */
    po::options_description_easy_init addOptions()
    {
      return m_options.add_options();
    }

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
};

#endif /* __ANACONDA_FRAMEWORK__UTILS__PLUGIN__SETTINGS_HPP__ */

/** End of file settings.hpp **/
