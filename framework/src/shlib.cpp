/**
 * @brief Contains implementation of classes representing shared libraries.
 *
 * A file containing implementation of classes representing shared libraries.
 *
 * @file      shlib.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-01-21
 * @version   0.1.1.1
 */

#include "shlib.h"

#include <assert.h>

#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#else
  #include "linux/dlutils.h"
#endif

/**
 * Constructs a SharedLibrary object.
 *
 * @param path A path to a shared library (.dll or .so file).
 * @param handle A handle representing a shared library.
 */
SharedLibrary::SharedLibrary(fs::path path, shlib_handle_t handle)
  : m_path(path), m_handle(handle)
{
}

/**
 * Constructs a SharedLibrary object from an existing SharedLibrary object.
 *
 * @param sl An object representing a shared library.
 */
SharedLibrary::SharedLibrary(const SharedLibrary& sl) : m_path(sl.m_path)
{
  // To close a shared library, one must delete the object representing it, so
  // we are always copying an opened shared library here, opening the library
  // again will only increment the library handle reference count, so no error
  // should occur here (everything is already loaded), but we are closing the
  // library when deleting the object, so we must open it during construction
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  m_handle = LoadLibrary(m_path.native().c_str());
#else
  m_handle = dlopen(m_path.native().c_str(), RTLD_LAZY|RTLD_GLOBAL);
#endif
  assert(m_handle == sl.m_handle); // Must be the same library which was copied
}

/**
 * Destroys a SharedLibrary object.
 */
SharedLibrary::~SharedLibrary()
{
  // Close the shared library, if the library's handle reference count is zero,
  // e.g., no other object representing this particular library is present, the
  // library will be unloaded from the memory of the process
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  FreeLibrary(m_handle);
#else
  dlclose(m_handle);
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
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  shlib_handle_t handle = LoadLibrary(path.native().c_str());
#else
  shlib_handle_t handle = dlopen(path.native().c_str(), RTLD_LAZY|RTLD_GLOBAL);
#endif

  if (handle == NULL)
  { // Shared library could not be loaded
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
    error = "LoadLibrary(" + path.native() + ") failed (error code "
      + boost::lexical_cast< std::string >(::GetLastError()) + ").";
#else
    error = "dlopen(" + path.native() + ") failed (" + dlerror() + ").";
#endif
    return NULL;
  }

  // Only here can be created a new shared library object (private constructor)
  return new SharedLibrary(path, handle);
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
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  return GetProcAddress(m_handle, symbol.c_str());
#else
  return dlsym(m_handle, symbol.c_str());
#endif
}

/**
 * Gets an address at which is a shared library loaded.
 *
 * @return The address at which is the shared library loaded or @em NULL if the
 *   address could not be resolved.
 */
void* SharedLibrary::getAddress()
{
#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
  // TODO: use LoadLibrary
#else
  // The shared library must be loaded here, so the address must be known
  return (void*)dl_get_sobj(absolute(this->m_path).c_str()).dlsi_addr;
#endif
}

/** End of file shlib.cpp **/
