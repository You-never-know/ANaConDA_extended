#
# AtomRace Plugin CMake Makefile Generation File
#
# File:      CMakeLists.txt
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-22
# Date:      Last Update 2012-02-22
# Version:   0.1
#

# Set the minimum CMake version needed
cmake_minimum_required(VERSION 2.8)

# Change the C++ compiler to the one chosen by the PIN framework
set(CMAKE_CXX_COMPILER ${CXX})

# Define a C++ project
project(anaconda-atomrace CXX)

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
  # A list of header files required to compile the plugin
  set(REQUIRED_HEADERS
    "anaconda.h")
  # Try to locate the anaconda frameworks's installation directories
  if (DEFINED ENV{ANACONDA_HOME})
    file(TO_CMAKE_PATH $ENV{ANACONDA_HOME}/include ANACONDA_INCLUDE_DIR)
  endif (DEFINED ENV{ANACONDA_HOME})
  # Check if all headers needed for the compilation are present
  foreach(HEADER_NAME ${REQUIRED_HEADERS})
    find_path(HEADER_DIR ${HEADER_NAME} ${ANACONDA_INCLUDE_DIR})

    # Add the directory containing the header file to include directories
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${HEADER_DIR})

    # If some header file is not present, abort the Makefile generation
    if (NOT HEADER_DIR)
      message(FATAL_ERROR "Header file '"${HEADER_NAME}"' not found.")
    endif (NOT HEADER_DIR)
  endforeach(HEADER_NAME)
  # Remove duplicate directories from the include directories
  list(REMOVE_DUPLICATES INCLUDE_DIRS)
endif (UNIX)

# Create a shared library (shared object or dynamic library)
add_library(anaconda-atomrace SHARED ${SOURCES})

# Check if the PIN library paths are already present in the linker flags or not
string(FIND ${PIN_LDFLAGS} ${PIN_LPATHS} SSPOS)

# If the library paths are not in the linker flags, add them there
if (SSPOS EQUAL -1)
  set(PIN_LDFLAGS "${PIN_LDFLAGS} ${PIN_LPATHS}")
endif (SSPOS EQUAL -1)

# Configure the build with the information obtained from the PIN's Makefile 
set_target_properties(anaconda-atomrace PROPERTIES
  # Set the compile flags contaning PIN include directories and definitions
  COMPILE_FLAGS ${PIN_CXXFLAGS}
  # Set the link flags contaning PIN library directories and symbol versions
  LINK_FLAGS ${PIN_LDFLAGS})

# Link the PIN libraries to the framework
target_link_libraries(anaconda-atomrace ${PIN_LIBS})

# Unix only
if (UNIX)
  # Include all directories with header files required to compile the framework
  include_directories(${INCLUDE_DIRS}) 
  # Compiler flags used in all build modes (position independent code, etc.)
  add_definitions(-fPIC)
  # Perform no optimizations and include debugging information in debug mode
  if (DEBUG)
    add_definitions(-g -DDEBUG)
  endif (DEBUG)
  # Set the target directory and change the framework's name
  set_target_properties(anaconda-atomrace PROPERTIES
    # Set the target directory where the framework will be compiled
    LIBRARY_OUTPUT_DIRECTORY .
    # Do not treat the framework as a library (do not prepend the lib prefix)
    PREFIX "")
endif (UNIX)

# Install the framework
install(TARGETS anaconda-atomrace DESTINATION lib/${TARGET_LONG})

# Install the header files required to use the framework
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# End of file CMakeLists.txt
