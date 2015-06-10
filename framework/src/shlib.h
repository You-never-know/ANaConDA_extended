/**
 * @brief Contains definitions of classes representing shared libraries.
 *
 * A file containing definitions of classes representing shared libraries.
 *
 * @file      shlib.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2015-06-10
 * @version   0.2.0.2
 */

#ifndef __PINTOOL_ANACONDA__SHLIB_H__
  #define __PINTOOL_ANACONDA__SHLIB_H__

#include <memory>

#include <boost/filesystem.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

// Type definitions
#ifdef TARGET_WINDOWS
  #define SHLIB_EXT ".dll"
#else
  #define SHLIB_EXT ".so"
#endif

/**
 * @brief A class representing a shared library.
 *
 * Represents a shared library, e.g. a dynamic library (.dll file) on Windows
 *   and a shared object (.so file) on Linux.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-06-30
 * @version   0.2.0.1
 */
class SharedLibrary
{
  private: // Declarations of opaque types
    struct Data;
  private: // Internal variables
#if defined(ECLIPSE_CDT_ENABLE_CODAN_FIXES)
    std::auto_ptr< Data > m_data; // CODAN does not parse unique_ptr correctly
#else
    std::unique_ptr< Data > m_data; //!< A structure holding internal data.
#endif
  public: // Static methods
    static SharedLibrary* Load(fs::path path, std::string& error);
  private: // Internal constructors
    SharedLibrary(Data* data);
  public: // Constructors
    SharedLibrary(const SharedLibrary& sl);
  public: // Destructors
    ~SharedLibrary();
  public: // Member methods
    void* resolve(const std::string& symbol);
  public: // Member methods for retrieving information about the shared library
    const fs::path& getPath();
    void* getAddress();
};

#endif /* __PINTOOL_ANACONDA__SHLIB_H__ */

/** End of file shlib.h **/
