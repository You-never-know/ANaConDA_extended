#
# Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of ANaConDA.
#
# ANaConDA is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# ANaConDA is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
#

#
# A CMake module which tries to find the libdie wrapper library.
#
# File:      Findlibdie-wrapper.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2019-02-04
# Version:   0.4.1
#

# Determine the target architecture (32-bit or 64-bit)
math(EXPR TARGET_ARCH_BITS "8 * ${CMAKE_SIZEOF_VOID_P}")

# Windows only
if (WIN32)
  # Load the module for correcting paths
  include(Paths)
  # Correct Cygwin paths to Windows paths
  CORRECT_PATHS(ENV{LIBDIE_WRAPPER_HOME} ENV{LIBDIE_WRAPPER_ROOT})

  # Determine where to search for the library on Windows
  set(LIBDIE_WRAPPER_INSTALL_LIBDIR "lib/win${TARGET_ARCH_BITS}")
endif (WIN32)

# Unix only
if (UNIX)
  # Determine where to search for the library on Linux
  set(LIBDIE_WRAPPER_INSTALL_LIBDIR "lib${TARGET_ARCH_BITS}")
endif (UNIX)

# First search the include directories specified by the environment variables
find_path(LIBDIE_WRAPPER_INCLUDE_DIR NAMES "libdie-wrapper/pin_die.h"
  PATHS "$ENV{LIBDIE_WRAPPER_HOME}" "$ENV{LIBDIE_WRAPPER_ROOT}" NO_DEFAULT_PATH
  PATH_SUFFIXES "include")
# If the headers were not found, search the default paths
find_path(LIBDIE_WRAPPER_INCLUDE_DIR NAMES "libdie-wrapper/pin_die.h")

# First search the library directories specified by the environment variables
find_library(LIBDIE_WRAPPER_LIBRARIES NAMES die-wrapper
  PATHS "$ENV{LIBDIE_WRAPPER_HOME}" "$ENV{LIBDIE_WRAPPER_ROOT}" NO_DEFAULT_PATH
  PATH_SUFFIXES "lib/${TARGET_LONG}" "${LIBDIE_WRAPPER_INSTALL_LIBDIR}" "lib")
# If the library was not found, search the default paths
find_library(LIBDIE_WRAPPER_LIBRARIES NAMES die-wrapper
  PATH_SUFFIXES "${TARGET_LONG}")

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBDIE-WRAPPER_FOUND to TRUE if the header files and library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdie-wrapper DEFAULT_MSG
  LIBDIE_WRAPPER_INCLUDE_DIR LIBDIE_WRAPPER_LIBRARIES)

# Do not show the variables set by the module in the CMake GUI
mark_as_advanced(LIBDIE_WRAPPER_INCLUDE_DIR LIBDIE_WRAPPER_LIBRARIES)

# End of file Findlibdie-wrapper.cmake
