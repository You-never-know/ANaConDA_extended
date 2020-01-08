/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions shared among various parts of the framework.
 *
 * A file containing definitions shared among various parts of the framework.
 *
 * @file      defs.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-05-28
 * @date      Last Update 2016-06-10
 * @version   0.1.5
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

#if NDEBUG
  #define ASSERT_VARIABLE(x) (void)(x)
#else
  #define ASSERT_VARIABLE(x)
#endif

#endif /* __PINTOOL_ANACONDA__DEFS_H__ */

/** End of file defs.h **/
