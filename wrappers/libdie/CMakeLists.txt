#
# PIN Debugging Information Extraction Library CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2012-02-23
# Version:   0.2
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8)

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
  # A list of header files required to compile the library
  set(REQUIRED_HEADERS
    "libdwarf/dwarf.h"
    "libdwarf/libdwarf.h"
    "die.h"
    "dwarf/dw_classes.h"
    "dwarf/dw_die.h"
    "dwarf/dw_visitors.h")
  # If the libdie library's home directory is specified, add its include dir
  if (DEFINED ENV{LIBDIE_HOME})
    file(TO_CMAKE_PATH $ENV{LIBDIE_HOME}/include LIBDIE_INCLUDE_DIR)
  endif (DEFINED ENV{LIBDIE_HOME})
  # Some custom modules are in the source directory, search there too
  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
  # Include the module for checking the availability of header files
  include(CheckHeaderFiles)
  # Check if all headers needed for the compilation are present
  CHECK_HEADER_FILES(${REQUIRED_HEADERS} REQUIRED PATHS ${LIBDIE_INCLUDE_DIR}
    $ENV{LIBDWARF_HOME})
  # Add the source files compiled only on UNIX
  aux_source_directory(src/dwarf SOURCES)
endif (UNIX)

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
