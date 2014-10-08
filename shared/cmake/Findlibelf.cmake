#
# A CMake module which tries to find the libelf library.
#
# File:      Findlibelf.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-25
# Date:      Last Update 2012-02-25
# Version:   0.1
#

# First search the include directories specified by the environment variables
find_path(LIBELF_INCLUDE_DIR NAMES libelf.h elf.h
  PATHS $ENV{LIBELF_HOME} $ENV{LIBELF_ROOT}
    $ENV{ELFUTILS_HOME} $ENV{ELFUTILS_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES include libelf)
# If the headers were not found, search the default paths
find_path(LIBELF_INCLUDE_DIR NAMES libelf.h elf.h
  PATH_SUFFIXES libelf)

# First search the library directories specified by the environment variables
find_library(LIBELF_LIBRARIES NAMES elf
  PATHS $ENV{LIBELF_HOME} $ENV{LIBELF_ROOT}
    $ENV{ELFUTILS_HOME} $ENV{ELFUTILS_ROOT} NO_DEFAULT_PATH
  PATH_SUFFIXES lib libelf)
# If the library was not found, search the default paths
find_library(LIBELF_LIBRARIES NAMES elf
  PATH_SUFFIXES lib libelf)

# Include the CMake module for handling the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
# Set LIBELF_FOUND to TRUE if the header files and the library were found
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libelf DEFAULT_MSG LIBELF_INCLUDE_DIR
  LIBELF_LIBRARIES)

# Do not show the varibles set by the module in the CMake GUI
mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARIES)

# End of file Findlibelf.cmake
