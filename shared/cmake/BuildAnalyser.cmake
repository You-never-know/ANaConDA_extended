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
# ANaConDA Analyser General CMake Makefile Generation File
#
# File:      BuildAnalyser.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-26
# Date:      Last Update 2019-02-01
# Version:   0.7.2
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the shared/cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../shared/cmake)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-${ANALYSER_NAME} CXX)

# Setup the build environment
include (SetupEnvironment)

# Collect source files compiled on all operating systems
aux_source_directory(src SOURCES)

# Windows only
if (WIN32)
  # Some compiler flags added by CMake conflict with flags set up by PIN
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  string(REPLACE /EHsc "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (WIN32)

# Create a shared library (shared object or dynamic library)
add_library(anaconda-${ANALYSER_NAME} SHARED ${SOURCES})

# Load the module for setting up the PIN framework
include(SetupPin)
# Configure the PIN framework so we can compile the analyser with it
SETUP_PIN(anaconda-${ANALYSER_NAME})

# Find the anaconda framework
find_package(anaconda-framework REQUIRED)
# Add the directory contaning anaconda header files to include directories
include_directories(${ANACONDA_FRAMEWORK_INCLUDE_DIR})
# Link the anaconda framework to the analyser
target_link_libraries(anaconda-${ANALYSER_NAME} ${ANACONDA_FRAMEWORK_LIBRARIES})

# Windows only
if (WIN32)
  # Boost libraries on Windows might or might not have the lib prefix
  set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
endif (WIN32)

# Load the module for setting up the Boost library
include(SetupBoost)
# Require the same version of the Boost library as the ANaConDA framework
SETUP_BOOST(anaconda-${ANALYSER_NAME} 1.46.0 filesystem program_options system)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC -std=c++11)
  # If compiling a 32-bit version on a 64-bit system, add additional flags
  if (CROSS_COMPILING_32_ON_64)
    add_definitions(-m32)
    # Link the analysers against 32-bit libraries (default are 64-bit)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
  endif (CROSS_COMPILING_32_ON_64)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g -DDEBUG)
  endif (DEBUG)
  # Set the target directory and change the analyser's name
  set_target_properties(anaconda-${ANALYSER_NAME} PROPERTIES
    # Set the target directory where the analyser will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the analyser as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Windows only
if (WIN32)
  # Load the module for generating path and symbol information for Eclipse
  include(GenerateScannerInfo)
  # Generate the information manually as automatic discovery seems not to work
  GENERATE_SCANNER_INFO("${PROJECT_BINARY_DIR}/scanner.info"
    FLAG_VARS CMAKE_CXX_FLAGS PIN_CXXFLAGS
    PATH_VARS ANACONDA_FRAMEWORK_INCLUDE_DIR Boost_INCLUDE_DIRS)
endif (WIN32)

# Install the analyser
install(TARGETS anaconda-${ANALYSER_NAME} DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the analyser
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file BuildAnalyser.cmake
