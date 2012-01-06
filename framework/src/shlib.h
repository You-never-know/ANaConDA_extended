/**
 * @brief Contains definitions of classes representing shared libraries.
 *
 * A file containing definitions of classes representing shared libraries.
 *
 * @file      shlib.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-01-06
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__SHLIB_H__
  #define __PINTOOL_ANACONDA__SHLIB_H__

#include <boost/filesystem.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

// Type definitions
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  typedef HMODULE shlib_handle_t;
#else
  typedef void* shlib_handle_t;
#endif

/**
 * @brief A class representing a shared library.
 *
 * Represents a shared library, e.g. a dynamic library (.dll file) on Windows
 *   and a shared object (.so file) on Linux.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-01-06
 * @version   0.1.1
 */
class SharedLibrary
{
  private: // User-defined variables
    fs::path m_path; //!< A path to a shared library (.dll or .so file).
  private: // Internal variables
    shlib_handle_t m_handle; //!< A handle representing a shared library.
  public: // Static methods
    static SharedLibrary* Load(fs::path path, std::string& error);
  private: // Internal constructors
    SharedLibrary(fs::path path, shlib_handle_t handle);
  public: // Constructors
    SharedLibrary(const SharedLibrary& sl);
  public: // Destructors
    ~SharedLibrary();
  public: // Member methods
    void* resolve(const std::string& symbol);
  public: // Member methods for retrieving information about the shared library
    void* getAddress();
  public: // Inline member methods
    /**
     * Gets a path to a shared library.
     *
     * @return The path to the shared library.
     */
    const fs::path& getPath()
    {
      return m_path;
    }
};

#endif /* __PINTOOL_ANACONDA__SHLIB_H__ */

/** End of file shlib.h **/
