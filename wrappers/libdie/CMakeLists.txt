#
# Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
# Debug Information Extraction Library Wrapper CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2019-02-01
# Version:   0.9
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the shared/cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../shared/cmake)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(libdie-wrapper CXX)

# Setup the build environment
include (SetupEnvironment)

# Collect source files compiled on all operating systems
aux_source_directory(src SOURCES)

# Unix only
if (UNIX)
  # Find the libdwarf library (need its headers to compile the libdie library)
  find_package(libdwarf REQUIRED)
  # Add the directory contaning libdwarf header files to include directories
  include_directories(${LIBDWARF_INCLUDE_DIR})
  # Add the source files compiled only on UNIX
  aux_source_directory(src/dwarf SOURCES)
endif (UNIX)

# Windows only
if (WIN32)
  # Some compiler flags added by CMake conflict with flags set up by PIN
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  string(REPLACE /EHsc "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  # Load the module for correcting paths
  include(Paths)
  # Correct the paths in compiler flags if necessary
  CORRECT_PATHS(PIN_CXXFLAGS)
endif (WIN32)

# Create a static library
add_library(die-wrapper STATIC ${SOURCES})

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(die-wrapper PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS})

# Find the libdie library
find_package(libdie REQUIRED)
# Add the directory contaning libdie header files to include directories
include_directories(${LIBDIE_INCLUDE_DIR})

# Load the module for setting up the Boost library
include(SetupBoost)
# The wrapper uses only some of the header files
SETUP_BOOST(die-wrapper 1.46.0)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
  # If compiling a 32-bit version on a 64-bit system, add additional flags
  if (CROSS_COMPILING_32_ON_64)
    add_definitions(-m32)
  endif (CROSS_COMPILING_32_ON_64)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-O0 -g3 -DDEBUG)
  endif (DEBUG)
  # Set the build directory where the library will be compiled
  set_target_properties(die-wrapper PROPERTIES
    # Set library's output directory
    ARCHIVE_OUTPUT_DIRECTORY .)
endif (UNIX)

# Windows only
if (WIN32)
  # Load the module for generating path and symbol information for Eclipse
  include(GenerateScannerInfo)
  # Generate the information manually as automatic discovery seems not to work
  GENERATE_SCANNER_INFO("${PROJECT_BINARY_DIR}/scanner.info"
    FLAG_VARS CMAKE_CXX_FLAGS PIN_CXXFLAGS
    PATH_VARS LIBDIE_INCLUDE_DIR)
endif (WIN32)

# Install the library
install(TARGETS die-wrapper DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install the header files required to use the library
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING
  PATTERN "*.h" PATTERN "dwarf" EXCLUDE PATTERN "util" EXCLUDE)

# End of file CMakeLists.txt
