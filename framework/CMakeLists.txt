#
# ANaConDA Framework CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2015-05-31
# Version:   0.9
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the shared/cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../shared/cmake)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-framework CXX)

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
aux_source_directory(src/callbacks SOURCES)
aux_source_directory(src/monitors SOURCES)
aux_source_directory(src/utils SOURCES)
aux_source_directory(src/utils/pin SOURCES)

# Unix only
if (UNIX)
  # Find the libelf library (need its headers to compile some helper functions)
  find_package(libelf REQUIRED)
  # Add the directory contaning libelf header files to include directories
  include_directories(${LIBELF_INCLUDE_DIR})
  # A list of additional header files required to compile the framework
  set(LIBELF_REQUIRED_INTERNAL_HEADERS
    gelf.h)
  # Load the module for checking existence of header files
  include(CheckHeaderExists)
  # Check if all headers needed for the compilation are present
  foreach(HEADER ${LIBELF_REQUIRED_INTERNAL_HEADERS})
    CHECK_HEADER_EXISTS(${HEADER} HEADER_DIR REQUIRED PATHS $ENV{LIBELF_HOME}
      $ENV{LIBELF_ROOT} PATH_SUFFIXES include)

    # Add the directory containing the header file to include directories
    include_directories(${HEADER_DIR})

    # Reuse the variable in the next iteration (remove it from cache)
    unset(HEADER_DIR)
  endforeach(HEADER)
  # Collect source files compiled only on UNIX
  aux_source_directory(src/utils/linux SOURCES)
endif (UNIX)

# Windows only
if (WIN32)
  # Some compiler flags added by CMake conflict with flags set up by PIN
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  string(REPLACE /EHsc "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (WIN32)

# Create a shared library (shared object or dynamic library)
add_library(anaconda-framework SHARED ${SOURCES})

# Add definitions used when compiling the debug version of the framework
if (DEBUG)
  add_definitions(-DDEBUG)
endif (DEBUG)

# Load the module for setting up the PIN framework
include(SetupPin)
# Configure the PIN framework so we can compile the ANaConDA framework with it
SETUP_PIN(anaconda-framework)

# Find the libdie-wrapper library
find_package(libdie-wrapper REQUIRED)
# Add the directory contaning libdie-wrapper header files to include directories
include_directories(${LIBDIE_WRAPPER_INCLUDE_DIR})
# Link the libdie-wrapper library to the framework
target_link_libraries(anaconda-framework ${LIBDIE_WRAPPER_LIBRARIES})

# Find the libdie library
find_package(libdie REQUIRED)
# Add the directory contaning libdie header files to include directories
include_directories(${LIBDIE_INCLUDE_DIR})
# Link the libdie library to the framework
target_link_libraries(anaconda-framework ${LIBDIE_LIBRARIES})

# Load the module for setting up the Boost library
include(SetupBoost)
# The framework uses filesystem V3, which is the default from version 1.46
SETUP_BOOST(anaconda-framework 1.46.0 date_time filesystem program_options
  system)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC -std=c++11)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g)
  endif (DEBUG)
  # Set the target directory and change the framework's name
  set_target_properties(anaconda-framework PROPERTIES
    # Set the target directory where the framework will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the framework as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Windows only
if (WIN32)
  # Load the module for generating path and symbol information for Eclipse
  include(GenerateScannerInfo)
  # Generate the information manually as automatic discovery seems not to work
  GENERATE_SCANNER_INFO("${PROJECT_BINARY_DIR}/scanner.info"
    FLAG_VARS CMAKE_CXX_FLAGS PIN_CXXFLAGS
    PATH_VARS LIBDIE_WRAPPER_INCLUDE_DIR LIBDIE_INCLUDE_DIR Boost_INCLUDE_DIRS)
endif (WIN32)

# Install the framework
install(TARGETS anaconda-framework DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the framework
install(DIRECTORY src/ DESTINATION include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# End of file CMakeLists.txt
