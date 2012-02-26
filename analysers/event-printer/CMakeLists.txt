#
# Event Printer Plugin CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-26
# Date:      Last Update 2012-02-26
# Version:   0.1
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8.3)

# Have custom modules in the directory contaning this file
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-event-printer CXX)

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

# Create a shared library (shared object or dynamic library)
add_library(anaconda-event-printer SHARED ${SOURCES})

# Check if the PIN library paths are already present in the linker flags or not
string(FIND ${PIN_LDFLAGS} ${PIN_LPATHS} SSPOS)

# If the library paths are not in the linker flags, add them there
if (SSPOS EQUAL -1)
  set(PIN_LDFLAGS "${PIN_LDFLAGS} ${PIN_LPATHS}")
endif (SSPOS EQUAL -1)

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(anaconda-event-printer PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS}
  # Set the link flags contaning PIN library directories and symbol versions
  LINK_FLAGS ${PIN_LDFLAGS})

# Link the PIN libraries to the plugin
target_link_libraries(anaconda-event-printer ${PIN_LIBS})

# Find the anaconda framework
find_package(anaconda REQUIRED)
# Add the directory contaning anaconda header files to include directories
include_directories(${ANACONDA_INCLUDE_DIR})
# Link the anaconda framework to the plugin
target_link_libraries(anaconda-event-printer ${ANACONDA_LIBRARIES})

# Unix only
if (UNIX)
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g -DDEBUG)
  endif (DEBUG)
  # Set the target directory and change the plugin's name
  set_target_properties(anaconda-event-printer PROPERTIES
    # Set the target directory where the plugin will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the plugin as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Install the plugin
install(TARGETS anaconda-event-printer DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the plugin
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file CMakeLists.txt
