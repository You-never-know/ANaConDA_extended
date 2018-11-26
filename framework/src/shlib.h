/*
 * Copyright (C) 2011-2018 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Contains definitions of classes representing shared libraries.
 *
 * A file containing definitions of classes representing shared libraries.
 *
 * @file      shlib.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2015-07-16
 * @version   0.4
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
 * @brief An enumeration of hidden shared libraries.
 */
typedef enum HiddenSharedLibrary_e
{
  PIN_FRAMEWORK, //!< Hidden PIN framework.
  ANACONDA_FRAMEWORK //!< Hidden ANaConDA framework.
} HiddenSharedLibrary;

/**
 * @brief A class representing a shared library.
 *
 * Represents a shared library, e.g. a dynamic library (.dll file) on Windows
 *   and a shared object (.so file) on Linux.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2015-07-16
 * @version   0.4
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
    static SharedLibrary* Get(HiddenSharedLibrary library);
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
  public: // Member methods for modifying the shared library
    void rebind(SharedLibrary* library);
};

#endif /* __PINTOOL_ANACONDA__SHLIB_H__ */

/** End of file shlib.h **/
