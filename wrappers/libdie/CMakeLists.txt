#
# Debug Information Extraction Library Wrapper CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2014-10-08
# Version:   0.6
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the shared/cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../shared/cmake)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(libdie-wrapper CXX)

# Allow only one build type, if more specified, then DEBUG > CHECKED > RELEASE
if (DEBUG)
  set (CHECKED FALSE)
  set (RELEASE FALSE)
elseif (CHECKED)
  set (DEBUG FALSE)
  set (RELEASE FALSE)
else (DEBUG)
  set (DEBUG FALSE)
  set (CHECKED FALSE)
  # If no build type is specified, presume the RELEASE build type
  set (RELEASE TRUE)
endif (DEBUG)

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

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
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
install(TARGETS die-wrapper DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the library
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file CMakeLists.txt
