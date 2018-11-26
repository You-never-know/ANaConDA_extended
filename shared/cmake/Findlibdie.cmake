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
# A CMake module which tries to find the libdie library.
#
# File:      Findlibdie.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2014-10-08
# Version:   0.2
#

# Load the module for correcting paths
include(Paths)
# Correct the paths if necessary
CORRECT_PATHS(ENV{LIBDIE_HOME} ENV{LIBDIE_ROOT})

# First search the include directories specified by the environment variables
find_path(LIBDIE_INCLUDE_DIR NAMES die.h
  PATHS $ENV{LIBDIE_HOME} $ENV{LIBDIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(LIBDIE_INCLUDE_DIR NAMES die.h)

# First search the library directories specified by the environment variables
find_library(LIBDIE_LIBRARIES NAMES die
  PATHS $ENV{LIBDIE_HOME} $ENV{LIBDIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES lib)
# If the library was not found, search the default paths
find_library(LIBDIE_LIBRARIES NAMES die)

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBDIE_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdie DEFAULT_MSG LIBDIE_INCLUDE_DIR
  LIBDIE_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(LIBDIE_INCLUDE_DIR LIBDIE_LIBRARIES)

# End of file Findlibdie.cmake
