/**
 * @brief Contains implementation of classes representing shared libraries.
 *
 * A file containing implementation of classes representing shared libraries.
 *
 * @file      shlib.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2015-07-16
 * @version   0.3
 */

#include "shlib.h"

#include <assert.h>

#ifdef TARGET_WINDOWS
  #include <windows.h>

  #include <boost/lexical_cast.hpp>

  #include "utils/windows/pe.h"
#else
  #include "utils/linux/dlutils.h"
#endif

#ifdef TARGET_WINDOWS
  typedef HMODULE shlib_handle_t;
#else
  typedef void* shlib_handle_t;
#endif

/**
 * @brief A structure holding internal data.
 */
struct SharedLibrary::Data
{
  fs::path path; //!< A path to a shared library (.dll or .so file).
  shlib_handle_t handle; //!< A handle representing a shared library.

  /**
   * Constructs a Data object.
   *
   * @param p A path to a shared library (.dll or .so file).
   * @param h A handle representing a shared library.
   */
  Data(fs::path p, shlib_handle_t h) : path(p), handle(h) {}
};

/**
 * Constructs a SharedLibrary object.
 *
 * @param data A structure holding internal data.
 */
SharedLibrary::SharedLibrary(Data* data) : m_data(data)
{
}

/**
 * Constructs a SharedLibrary object from an existing SharedLibrary object.
 *
 * @param sl An object representing a shared library.
 */
SharedLibrary::SharedLibrary(const SharedLibrary& sl)
{
  // To close a shared library, one must delete the object representing it, so
  // we are always copying an opened shared library here, opening the library
  // again will only increment the library handle reference count, so no error
  // should occur here (everything is already loaded), but we are closing the
  // library when deleting the object, so we must open it during construction
  m_data.reset(new Data(sl.m_data->path,
#ifdef TARGET_WINDOWS
    LoadLibrary(sl.m_data->path.string().c_str())));
#else
    dlopen(sl.m_data->path.string().c_str(), RTLD_LAZY|RTLD_GLOBAL)));
#endif
  // Must be the same library which was copied
  assert(m_data->handle == sl.m_data->handle);
}

/**
 * Destroys a SharedLibrary object.
 */
SharedLibrary::~SharedLibrary()
{
  // Close the shared library, if the library's handle reference count is zero,
  // e.g., no other object representing this particular library is present, the
  // library will be unloaded from the memory of the process
#ifdef TARGET_WINDOWS
  FreeLibrary(m_data->handle);
#else
  dlclose(m_data->handle);
#endif
}

/**
 * Loads a shared library (dynamic library on Windows, shared object on Linux).
 *
 * @param path A path to the shared library (.dll or .so file).
 * @param error A reference to a string where the description of an error will
 *   be stored if the shared library could not be loaded.
 * @return A pointer to an object representing the loaded shared library or @em
 *   NULL if the shared library could not be loaded.
 */
SharedLibrary* SharedLibrary::Load(fs::path path, std::string& error)
{
#ifdef TARGET_WINDOWS
  shlib_handle_t handle = LoadLibrary(path.string().c_str());
#else
  shlib_handle_t handle = dlopen(path.string().c_str(), RTLD_LAZY|RTLD_GLOBAL);
#endif

  if (handle == NULL)
  { // Shared library could not be loaded
#ifdef TARGET_WINDOWS
    error = "LoadLibrary(" + path.string() + ") failed (error code "
      + boost::lexical_cast< std::string >(::GetLastError()) + ").";
#else
    error = "dlopen(" + path.string() + ") failed (" + dlerror() + ").";
#endif
    return NULL;
  }

  // Only here can be created a new shared library object (private constructor)
  return new SharedLibrary(new Data(path, handle));
}

/**
 * Resolves a symbol in a shared library.
 *
 * @param symbol A name of the symbol.
 * @return Pointer to the symbol or \em NULL if no symbol with the specified
 *   name was found.
 */
void* SharedLibrary::resolve(const std::string& symbol)
{
#ifdef TARGET_WINDOWS
  return GetProcAddress(m_data->handle, symbol.c_str());
#else
  return dlsym(m_data->handle, symbol.c_str());
#endif
}

/**
 * Gets a path to a shared library.
 *
 * @return The path to the shared library.
 */
const fs::path& SharedLibrary::getPath()
{
  return m_data->path;
}

/**
 * Gets an address at which is a shared library loaded.
 *
 * @return The address at which is the shared library loaded or @em NULL if the
 *   address could not be resolved.
 */
void* SharedLibrary::getAddress()
{
#ifdef TARGET_WINDOWS
  // The value of the handle is the address at which is the library loaded
  return (void*)m_data->handle;
#else
  // The shared library must be loaded here, so the address must be known
  return (void*)dl_get_sobj(absolute(m_data->path).c_str()).dlsi_addr;
#endif
}

/**
 * Rebinds the shared library to a specified shared library, i.e., rebinds all
 *   imported functions of the shared library to the functions exported by the
 *   specified shared library.
 *
 * @param library A shared library whose exported functions should this shared
 *   library call instead of the ones set by the Windows loader.
 */
void SharedLibrary::rebind(SharedLibrary* library)
{
#ifdef TARGET_WINDOWS
  redirectCalls(m_data->handle, library->m_data->handle);
#endif
}

/** End of file shlib.cpp **/
