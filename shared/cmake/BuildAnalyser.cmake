#
# ANaConDA Analyser General CMake Makefile Generation File
#
# File:      BuildAnalyser.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-26
# Date:      Last Update 2015-06-01
# Version:   0.6
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Search for custom modules in the shared/cmake directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../shared/cmake)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-${ANALYSER_NAME} CXX)

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
SETUP_BOOST(anaconda-${ANALYSER_NAME} 1.46.0 system)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC -std=c++11)
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
