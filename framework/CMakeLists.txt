#
# ANaConDA Framework CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-21
# Date:      Last Update 2013-05-24
# Version:   0.2.10.1
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Have custom modules in the directory contaning this file
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

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
aux_source_directory(src/pin SOURCES)
aux_source_directory(src/util SOURCES)

# Unix only
if (UNIX)
  # Find the libelf library (need its headers to compile some helper functions)
  find_package(libelf REQUIRED)
  # Add the directory contaning libelf header files to include directories
  include_directories(${LIBELF_INCLUDE_DIR})
  # Collect source files compiled only on UNIX
  aux_source_directory(src/linux SOURCES)
endif (UNIX)

# Windows only
if (WIN32)
  # Some compiler flags added by CMake conflict with flags set up by PIN
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  string(REPLACE /EHsc "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif (WIN32)

# Create a shared library (shared object or dynamic library)
add_library(anaconda SHARED ${SOURCES})

# Add definitions used when compiling the debug version of the framework
if (DEBUG)
  add_definitions(-DDEBUG)
endif (DEBUG)

# Check if the PIN library paths are already present in the linker flags or not
string(FIND ${PIN_LDFLAGS} ${PIN_LPATHS} SSPOS)

# If the library paths are not in the linker flags, add them there
if (SSPOS EQUAL -1)
  set(PIN_LDFLAGS "${PIN_LDFLAGS} ${PIN_LPATHS}")
endif (SSPOS EQUAL -1)

# Unix only
if (UNIX)
  # PIN hides most of the symbols, but we need to export the framework API
  string(REGEX REPLACE "-Wl,--version-script=.*/pintool.ver"
    "-Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/anaconda.api"
    PIN_LDFLAGS ${PIN_LDFLAGS})
endif (UNIX)

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(anaconda PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS}
  # Set the link flags contaning PIN library directories and symbol versions
  LINK_FLAGS ${PIN_LDFLAGS})

# Link the PIN libraries to the framework
target_link_libraries(anaconda ${PIN_LIBS})

# Find the pinlib-die library
find_package(pinlib-die REQUIRED)
# Add the directory contaning pinlib-die header files to include directories
include_directories(${PINLIB-DIE_INCLUDE_DIR})
# Link the pinlib-die library to the framework
target_link_libraries(anaconda ${PINLIB-DIE_LIBRARIES})

# Find the libdie library
find_package(libdie REQUIRED)
# Add the directory contaning libdie header files to include directories
include_directories(${LIBDIE_INCLUDE_DIR})
# Link the libdie library to the framework
target_link_libraries(anaconda ${LIBDIE_LIBRARIES})

# Do not need the multithreaded version for now and it is not always available
set(Boost_USE_MULTITHREADED FALSE)
# Boost's filesystem V3 is used, which is from version 1.46 as default
find_package(Boost 1.46.0 COMPONENTS date_time filesystem program_options regex
  system)

# If Boost 1.46 or newer is found, add the required includes and libraries
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(anaconda ${Boost_LIBRARIES})
else (Boost_FOUND)
  message(FATAL_ERROR "Boost not found.")
endif (Boost_FOUND)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC -std=c++0x)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g)
  endif (DEBUG)
  # Set the target directory and change the framework's name
  set_target_properties(anaconda PROPERTIES
    # Set the target directory where the framework will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the framework as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Windows only
if (WIN32)
  # Have custom modules in the directory contaning this file
  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
  # Load the module for generating path and symbol information for Eclipse
  include(GenerateScannerInfo)
  # Generate the information manually as automatic discovery seems not to work
  GENERATE_SCANNER_INFO("${PROJECT_BINARY_DIR}/scanner.info"
    FLAG_VARS CMAKE_CXX_FLAGS PIN_CXXFLAGS
    PATH_VARS PINLIB-DIE_INCLUDE_DIR LIBDIE_INCLUDE_DIR Boost_INCLUDE_DIRS)
endif (WIN32)

# Install the framework
install(TARGETS anaconda DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the framework
install(DIRECTORY src/ DESTINATION include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# End of file CMakeLists.txt
