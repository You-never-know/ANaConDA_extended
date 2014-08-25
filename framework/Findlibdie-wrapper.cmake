#
# A CMake module which tries to find the pinlib-die library.
#
# File:      Findpinlib-die.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2012-02-25
# Version:   0.1
#

# First search the include directories specified by the environment variables
find_path(PINLIB-DIE_INCLUDE_DIR NAMES pin_die.h
  PATHS $ENV{PINLIB_DIE_HOME} $ENV{PINLIB_DIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(PINLIB-DIE_INCLUDE_DIR NAMES pin_die.h)

# First search the library directories specified by the environment variables
find_library(PINLIB-DIE_LIBRARIES NAMES pin-die
  PATHS $ENV{PINLIB_DIE_HOME} $ENV{PINLIB_DIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES lib ${TARGET_LONG} lib/${TARGET_LONG})
# If the library was not found, search the default paths
find_library(PINLIB-DIE_LIBRARIES NAMES pin-die
  PATH_SUFFIXES ${TARGET_LONG})

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set PINLIB-DIE_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(pinlib-die DEFAULT_MSG PINLIB-DIE_INCLUDE_DIR
  PINLIB-DIE_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(PINLIB-DIE_INCLUDE_DIR PINLIB-DIE_LIBRARIES)

# End of file Findpinlib-die.cmake
