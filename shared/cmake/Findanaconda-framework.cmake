#
# Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
# A CMake module which tries to find the ANaConDA framework.
#
# File:      Findanaconda-framework.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2015-06-01
# Version:   0.4
#

# Windows only, correct Cygwin paths to Windows paths
if (WIN32)
  # Load the module for correcting paths
  include(Paths)
  # Correct the paths to the ANaConDA framework if necessary
  CORRECT_PATHS(ENV{ANACONDA_FRAMEWORK_HOME} ENV{ANACONDA_FRAMEWORK_ROOT})
endif (WIN32)

# First search the include directories specified by the environment variables
find_path(ANACONDA_FRAMEWORK_INCLUDE_DIR NAMES anaconda.h
  PATHS $ENV{ANACONDA_FRAMEWORK_HOME} $ENV{ANACONDA_FRAMEWORK_ROOT}
  NO_DEFAULT_PATH PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(ANACONDA_FRAMEWORK_INCLUDE_DIR NAMES anaconda.h)

# Save the original library prefixes
set(ORIG_CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})

# The ANaConDA framework is not treated as a library (i.e. have no lib prefix)
set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")

# First search the library directories specified by the environment variables
find_library(ANACONDA_FRAMEWORK_LIBRARIES NAMES anaconda-framework
  PATHS $ENV{ANACONDA_FRAMEWORK_HOME} $ENV{ANACONDA_FRAMEWORK_ROOT}
  NO_DEFAULT_PATH PATH_SUFFIXES lib ${TARGET_LONG} lib/${TARGET_LONG})
# If the library was not found, search the default paths
find_library(ANACONDA_FRAMEWORK_LIBRARIES NAMES anaconda-framework
  PATH_SUFFIXES ${TARGET_LONG})

# Restore the original library prefixes
set(CMAKE_FIND_LIBRARY_PREFIXES ${ORIG_CMAKE_FIND_LIBRARY_PREFIXES})

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set PINLIB-DIE_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(anaconda DEFAULT_MSG
  ANACONDA_FRAMEWORK_INCLUDE_DIR ANACONDA_FRAMEWORK_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(ANACONDA_FRAMEWORK_INCLUDE_DIR ANACONDA_FRAMEWORK_LIBRARIES)

# End of file Findanaconda-framework.cmake
