#
# Copyright (C) 2015-2020 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of ANaConDA.
#
# ANaConDA is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# ANaConDA is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
#

#
# A CMake module which sets up the PIN framework.
#
# File:      SetupPin.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2015-05-31
# Date:      Last Update 2015-08-12
# Version:   0.2.1
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
      "-Wl,--version-script=$ENV{SOURCE_DIR}/framework/anaconda.api"
      PIN_LDFLAGS ${PIN_LDFLAGS})
  endif (UNIX)

  # Windows only, correct Cygwin paths to Windows paths
  if (WIN32)
    # If setting up PIN for some analyser, disable pintool-specific config
    if (NOT "${project}" STREQUAL "anaconda-framework")
      # Do not export main or PIN will think the analyser is a pintool
      string(REGEX REPLACE "/EXPORT:main" "" PIN_LDFLAGS ${PIN_LDFLAGS})
      # Use the standard DllMain entry function for DLL initialization
      string(REGEX REPLACE "/ENTRY:Ptrace_DllMainCRTStartup" "" PIN_LDFLAGS
        ${PIN_LDFLAGS})
      # Do not try to load the analyser at a specific address, any will do
      string(REGEX REPLACE "/BASE:0xC5000000" "" PIN_LDFLAGS ${PIN_LDFLAGS})
    endif (NOT "${project}" STREQUAL "anaconda-framework")
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
