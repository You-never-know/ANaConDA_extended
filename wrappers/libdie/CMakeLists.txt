#
# PIN Debugging Information Extraction Library CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2012-05-25
# Version:   0.3.1
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(pinlib-die CXX)

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
  # Have custom modules in the directory contaning this file
  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
  # Find the libdwarf library (need its headers to compile the libdie library)
  find_package(libdwarf REQUIRED)
  # Add the directory contaning libdwarf header files to include directories
  include_directories(${LIBDWARF_INCLUDE_DIR})
  # Find the libdie library
  find_package(libdie REQUIRED)
  # Add the directory contaning libdie header files to include directories
  include_directories(${LIBDIE_INCLUDE_DIR})
  # Add the source files compiled only on UNIX
  aux_source_directory(src/dwarf SOURCES)
endif (UNIX)

# Windows only
if (WIN32)
  # Some compiler flags added by CMake conflict with flags set up by PIN
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  string(REPLACE /EHsc "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (WIN32)

# Create a static library
add_library(pin-die STATIC ${SOURCES})

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(pin-die PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS})

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-O0 -g3 -DDEBUG)
  endif (DEBUG)
  # Set the build directory where the library will be compiled
  set_target_properties(pin-die PROPERTIES
    # Set library's output directory
    ARCHIVE_OUTPUT_DIRECTORY .)
endif (UNIX)

# Install the library
install(TARGETS pin-die DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the library
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file CMakeLists.txt
