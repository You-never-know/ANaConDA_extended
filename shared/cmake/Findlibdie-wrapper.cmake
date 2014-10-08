#
# A CMake module which tries to find the libdie wrapper library.
#
# File:      Findlibdie-wrapper.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2014-10-08
# Version:   0.3
#

# Load the module for correcting paths
include(Paths)
# Correct the paths if necessary
CORRECT_PATHS(ENV{LIBDIE_WRAPPER_HOME} ENV{LIBDIE_WRAPPER_ROOT})

# First search the include directories specified by the environment variables
find_path(LIBDIE_WRAPPER_INCLUDE_DIR NAMES pin_die.h
  PATHS $ENV{LIBDIE_WRAPPER_HOME} $ENV{LIBDIE_WRAPPER_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(LIBDIE_WRAPPER_INCLUDE_DIR NAMES pin_die.h)

# First search the library directories specified by the environment variables
find_library(LIBDIE_WRAPPER_LIBRARIES NAMES die-wrapper
  PATHS $ENV{LIBDIE_WRAPPER_HOME} $ENV{LIBDIE_WRAPPER_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES lib ${TARGET_LONG} lib/${TARGET_LONG})
# If the library was not found, search the default paths
find_library(LIBDIE_WRAPPER_LIBRARIES NAMES die-wrapper
  PATH_SUFFIXES ${TARGET_LONG})

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBDIE-WRAPPER_FOUND to TRUE if the header files and library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdie-wrapper DEFAULT_MSG
  LIBDIE_WRAPPER_INCLUDE_DIR LIBDIE_WRAPPER_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(LIBDIE_WRAPPER_INCLUDE_DIR LIBDIE_WRAPPER_LIBRARIES)

# End of file Findlibdie-wrapper.cmake
