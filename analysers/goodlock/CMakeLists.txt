#
# GoodLock Plugin CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-03-09
# Date:      Last Update 2012-06-22
# Version:   0.1.1
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Have custom modules in the directory contaning this file
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-goodlock CXX)

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
add_library(anaconda-goodlock SHARED ${SOURCES})

# Check if the PIN library paths are already present in the linker flags or not
string(FIND ${PIN_LDFLAGS} ${PIN_LPATHS} SSPOS)

# If the library paths are not in the linker flags, add them there
if (SSPOS EQUAL -1)
  set(PIN_LDFLAGS "${PIN_LDFLAGS} ${PIN_LPATHS}")
endif (SSPOS EQUAL -1)

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(anaconda-goodlock PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS}
  # Set the link flags contaning PIN library directories and symbol versions
  LINK_FLAGS ${PIN_LDFLAGS})

# Link the PIN libraries to the plugin
target_link_libraries(anaconda-goodlock ${PIN_LIBS})

# Find the anaconda framework
find_package(anaconda REQUIRED)
# Add the directory contaning anaconda header files to include directories
include_directories(${ANACONDA_INCLUDE_DIR})
# Link the anaconda framework to the plugin
target_link_libraries(anaconda-goodlock ${ANACONDA_LIBRARIES})

# Windows only
if (WIN32)
  # Boost libraries on Windows might or might not have the lib prefix
  set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
endif (WIN32)

# Do not need the multithreaded version for now and it is not always available
set(Boost_USE_MULTITHREADED FALSE)
# Boost headers are included in some of the framework's headers
find_package(Boost 1.46.0 COMPONENTS system)

# If Boost 1.46 or newer is found, add the required includes and libraries
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(anaconda-goodlock ${Boost_LIBRARIES})
else (Boost_FOUND)
  message(FATAL_ERROR "Boost not found.")
endif (Boost_FOUND)

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g -DDEBUG)
  endif (DEBUG)
  # Set the target directory and change the plugin's name
  set_target_properties(anaconda-goodlock PROPERTIES
    # Set the target directory where the plugin will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the plugin as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Install the plugin
install(TARGETS anaconda-goodlock DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the plugin
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file CMakeLists.txt
