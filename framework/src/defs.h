/**
 * @brief Contains definitions shared among various parts of the framework.
 *
 * A file containing definitions shared among various parts of the framework.
 *
 * @file      defs.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-05-28
 * @date      Last Update 2015-06-01
 * @version   0.1.4
 */

#ifndef __PINTOOL_ANACONDA__DEFS_H__
  #define __PINTOOL_ANACONDA__DEFS_H__

#if defined(_MSC_VER)
  // Microsoft Visual C++ compiler does not have the constexpr support yet
  #define ANACONDA_HAS_CONSTEXPR 0
  #define CONSTEXPR
#else
  // If the compiler supports constexpr, use it to achieve better performance
  #define ANACONDA_HAS_CONSTEXPR 1
  #define CONSTEXPR constexpr
#endif

#ifdef TARGET_WINDOWS
  // On Windows, the API functions will be exported using the dllexport flag
  #define API_FUNCTION __declspec(dllexport)
#else
  // On Linux, the API functions will be exported using -Wl,--version-script
  #define API_FUNCTION
#endif

// Plugin functions called from the framework should be like C API functions
#define PLUGIN_FUNCTION(name) \
  extern "C" \
  API_FUNCTION \
  void name

// Define macros for the plugin entry and exit functions (init and finish)
#define PLUGIN_INIT_FUNCTION PLUGIN_FUNCTION(init)
#define PLUGIN_FINISH_FUNCTION PLUGIN_FUNCTION(finish)

// Definitions of error codes
#define EREGISTERED 200

#ifdef TARGET_WINDOWS
  #define PATH_SEP_CHAR '\\'
  #define PATH_SEP_CHAR_ALT '/'
#else
  #define PATH_SEP_CHAR '/'
  #define PATH_SEP_CHAR_ALT '\\'
#endif

#endif /* __PINTOOL_ANACONDA__DEFS_H__ */

/** End of file defs.h **/
