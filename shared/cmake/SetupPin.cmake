#
# A CMake module which sets up the PIN framework.
#
# File:      SetupPin.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2015-05-31
# Date:      Last Update 2015-05-31
# Version:   0.1
#

#
# Sets up the PIN framework for a specific project. This includes configuring
#   and correcting compiler and linker flags to be able to compile the project
#   with the PIN framework.
#
# SETUP_PIN(<project>)
#
MACRO(SETUP_PIN project)
  # Check if the PIN library paths are present in the linker flags or not
  string(FIND ${PIN_LDFLAGS} ${PIN_LPATHS} SSPOS)

  # If the library paths are not in the linker flags, add them there
  if (SSPOS EQUAL -1)
    set(PIN_LDFLAGS "${PIN_LDFLAGS} ${PIN_LPATHS}")
  endif (SSPOS EQUAL -1)

  # Unix only, export the framework API
  if (UNIX)
    # PIN hides most of the symbols, but we need to export the framework API
    string(REGEX REPLACE "-Wl,--version-script=.*/pintool.ver"
      "-Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/anaconda.api"
      PIN_LDFLAGS ${PIN_LDFLAGS})
  endif (UNIX)

  # Windows only, correct Cygwin paths to Windows paths
  if (WIN32)
    # Load the module for correcting paths
    include(Paths)
    # Correct the paths to PIN headers and libraries if necessary
    CORRECT_PATHS(PIN_CXXFLAGS PIN_LDFLAGS)
  endif (WIN32)

  # Configure the build with the information obtained from the PIN's Makefile 
  set_target_properties(${project} PROPERTIES
    # Set the compile flags contaning PIN include directories and definitions
    COMPILE_FLAGS ${PIN_CXXFLAGS}
    # Set the link flags contaning PIN library directories and symbol versions
    LINK_FLAGS ${PIN_LDFLAGS})

  # Link the PIN libraries to the project
  target_link_libraries(${project} ${PIN_LIBS})
ENDMACRO(SETUP_PIN)

# End of file SetupPin.cmake
