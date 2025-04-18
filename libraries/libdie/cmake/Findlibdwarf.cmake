#
# Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of libdie.
#
# libdie is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# libdie is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libdie. If not, see <http://www.gnu.org/licenses/>.
#

#
# A CMake module which tries to find the libdwarf library.
#
# File:      Findlibdwarf.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-24
# Date:      Last Update 2012-02-25
# Version:   0.1
#

# First search the include directories specified by the environment variables
find_path(LIBDWARF_INCLUDE_DIR NAMES libdwarf/libdwarf.h libdwarf/dwarf.h
  PATHS $ENV{LIBDWARF_HOME} $ENV{LIBDWARF_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(LIBDWARF_INCLUDE_DIR NAMES libdwarf/libdwarf.h libdwarf/dwarf.h)

# First search the library directories specified by the environment variables
find_library(LIBDWARF_LIBRARIES NAMES dwarf
  PATHS $ENV{LIBDWARF_HOME} $ENV{LIBDWARF_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES libdwarf)
# If the library was not found, search the default paths
find_library(LIBDWARF_LIBRARIES NAMES dwarf PATH_SUFFIXES libdwarf)

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBDWARF_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdwarf DEFAULT_MSG LIBDWARF_INCLUDE_DIR
  LIBDWARF_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(LIBDWARF_INCLUDE_DIR LIBDWARF_LIBRARIES)

# End of file Findlibdwarf.cmake
