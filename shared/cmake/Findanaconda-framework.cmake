#
# A CMake module which tries to find the ANaConDA framework.
#
# File:      Findanaconda-framework.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2014-11-06
# Version:   0.2
#

# First search the include directories specified by the environment variables
find_path(ANACONDA_FRAMEWORK_INCLUDE_DIR NAMES anaconda.h
  PATHS $ENV{ANACONDA_FRAMEWORK_HOME} $ENV{ANACONDA_FRAMEWORK_ROOT}
  NO_DEFAULT_PATH PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(ANACONDA_FRAMEWORK_INCLUDE_DIR NAMES anaconda.h)

# Save the original library prefixes
set(ORIG_CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})

# The ANaConDA framework is not treated as a library (i.e. have no lib prefix)
set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")

# First search the library directories specified by the environment variables
find_library(ANACONDA_FRAMEWORK_LIBRARIES NAMES anaconda
  PATHS $ENV{ANACONDA_FRAMEWORK_HOME} $ENV{ANACONDA_FRAMEWORK_ROOT}
  NO_DEFAULT_PATH PATH_SUFFIXES lib ${TARGET_LONG} lib/${TARGET_LONG})
# If the library was not found, search the default paths
find_library(ANACONDA_FRAMEWORK_LIBRARIES NAMES anaconda
  PATH_SUFFIXES ${TARGET_LONG})

# Restore the original library prefixes
set(CMAKE_FIND_LIBRARY_PREFIXES ${ORIG_CMAKE_FIND_LIBRARY_PREFIXES})

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set PINLIB-DIE_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(anaconda DEFAULT_MSG
  ANACONDA_FRAMEWORK_INCLUDE_DIR ANACONDA_FRAMEWORK_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(ANACONDA_FRAMEWORK_INCLUDE_DIR ANACONDA_FRAMEWORK_LIBRARIES)

# End of file Findanaconda-framework.cmake
