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
# Debugging Information Extraction Library CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-17
# Date:      Last Update 2019-01-29
# Version:   0.4.3
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Define a C++ project
project(libdie CXX)

# Setup the build environment
include (SetupEnvironment)

# Collect source files compiled on all operating systems
aux_source_directory(src SOURCES)

# Unix only
if (UNIX)
  # Find the libdwarf library
  find_package(libdwarf REQUIRED)
  # Add the directory contaning libdwarf header files to include directories
  include_directories(${LIBDWARF_INCLUDE_DIR})
  # A list of additional header files required to compile the library
  set(LIBDWARF_REQUIRED_INTERNAL_HEADERS
    libdwarf/config.h
    libdwarf/dwarf_alloc.h
    libdwarf/dwarf_base_types.h
    libdwarf/dwarf_opaque.h
    libdwarf/libdwarfdefs.h)
  # Load the module for checking existence of header files
  include(CheckHeaderExists)
  # Check if all headers needed for the compilation are present
  foreach(HEADER ${LIBDWARF_REQUIRED_INTERNAL_HEADERS})
    CHECK_HEADER_EXISTS(${HEADER} HEADER_DIR REQUIRED PATHS $ENV{LIBDWARF_HOME}
      $ENV{LIBDWARF_ROOT} PATH_SUFFIXES include)

    # Add the directory containing the header file to include directories
    include_directories(${HEADER_DIR})

    # Reuse the variable in the next iteration (remove it from cache)
    unset(HEADER_DIR)
  endforeach(HEADER)
  # Collect source files compiled only on UNIX
  aux_source_directory(src/dwarf SOURCES)
endif (UNIX)

# Create a static library
add_library(die STATIC ${SOURCES})

# Unix only
if (UNIX)
  # Find the Boost libraries
  find_package(Boost 1.46.0 REQUIRED)
  # Only the header files are needed
  include_directories(${Boost_INCLUDE_DIRS})
endif (UNIX)

# Load the module for generation build information
include(GenerateBuildInfo)
# Generate build information for the library
GENERATE_BUILD_INFO(PREFIX LIBDIE FILES src/version.cpp)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC -Wall -fmessage-length=0 -DTARGET_LINUX)
  # If compiling a 32-bit version on a 64-bit system, add additional flags
  if (CROSS_COMPILING_32_ON_64)
    add_definitions(-m32)
  endif (CROSS_COMPILING_32_ON_64)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-O0 -g3 -DDEBUG)
  endif (DEBUG)
  # Set the build directory where the library will be compiled
  set_target_properties(die PROPERTIES
    # Set library's output directory
    ARCHIVE_OUTPUT_DIRECTORY .)
endif (UNIX)

# Windows only
if (WIN32)
  # Set the default installation directory for libraries explicitly
  if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      # 64-bit pointers means 64-bit Windows
      set(CMAKE_INSTALL_LIBDIR "lib/win64")
    else ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      # Else we are running 32-bit Windows
      set(CMAKE_INSTALL_LIBDIR "lib/win32")
    endif ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  endif (NOT DEFINED CMAKE_INSTALL_LIBDIR)

  # Load the module for correcting paths
  include(Paths)
  # Correct the installation prefix and other installation directories
  CORRECT_PATHS(CMAKE_INSTALL_PREFIX CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR)
endif (WIN32)

# Load the module that defines standard installation directories
include(GNUInstallDirs)

# Install the library
install(TARGETS die DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install the header files required to use the library on any system
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h" PATTERN "dwarf" EXCLUDE)

# Unix only
if (UNIX)
  # Install additional header files needed to use the library on Linux
  install(DIRECTORY src/dwarf DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")
endif (UNIX)

# End of file CMakeLists.txt
