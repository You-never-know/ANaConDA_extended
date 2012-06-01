/**
 * @brief Contains definitions shared among various parts of the framework.
 *
 * A file containing definitions shared among various parts of the framework.
 *
 * @file      defs.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-05-28
 * @date      Last Update 2012-06-01
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__DEFS_H__
  #define __PINTOOL_ANACONDA__DEFS_H__

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

#endif /* __PINTOOL_ANACONDA__DEFS_H__ */

/** End of file defs.h **/
