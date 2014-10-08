#
# A CMake module which tries to find the libdie library.
#
# File:      Findlibdie.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2012-02-25
# Version:   0.1
#

# First search the include directories specified by the environment variables
find_path(LIBDIE_INCLUDE_DIR NAMES die.h
  PATHS $ENV{LIBDIE_HOME} $ENV{LIBDIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include)
# If the headers were not found, search the default paths
find_path(LIBDIE_INCLUDE_DIR NAMES die.h)

# First search the library directories specified by the environment variables
find_library(LIBDIE_LIBRARIES NAMES die
  PATHS $ENV{LIBDIE_HOME} $ENV{LIBDIE_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES lib)
# If the library was not found, search the default paths
find_library(LIBDIE_LIBRARIES NAMES die)

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBDIE_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdie DEFAULT_MSG LIBDIE_INCLUDE_DIR
  LIBDIE_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(LIBDIE_INCLUDE_DIR LIBDIE_LIBRARIES)

# End of file Findlibdie.cmake
