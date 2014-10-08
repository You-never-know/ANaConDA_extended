#
# A CMake module which corrects paths when building in mixed environment.
#
# File:      Paths.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2014-10-08
# Date:      Last Update 2014-10-08
# Version:   0.2
#

#
# Corrects paths when building in mixed environment, e.g., in Cygwin on Windows
#   using the Visual Studio compiler and other Windows tools.
#
# CORRECT_PATHS(<var1> [<var2> ...])
#
MACRO(CORRECT_PATHS)
  foreach(VAR ${ARGN})
    if (DEFINED ${VAR} AND NOT "${VAR}" STREQUAL "")
      if (${VAR} MATCHES "^ENV{(.*)}$")
        set(PATH $ENV{${CMAKE_MATCH_1}})
      else (${VAR} MATCHES "^ENV{(.*)}$")
        set(PATH ${${VAR}})
      endif (${VAR} MATCHES "^ENV{(.*)}$")

      string(REGEX REPLACE "([^ ]*)/cygdrive/([a-z])([^ ]*)" "\\1\\2:\\3" PATH ${PATH})

      if (${VAR} MATCHES "^ENV{(.*)}$")
        set(ENV{${CMAKE_MATCH_1}} ${PATH})
      else (${VAR} MATCHES "^ENV{(.*)}$")
        set(${VAR} ${PATH})
      endif (${VAR} MATCHES "^ENV{(.*)}$")
    endif (DEFINED ${VAR} AND NOT "${VAR}" STREQUAL "")
  endforeach(VAR ${ARGN})
ENDMACRO(CORRECT_PATHS)

# End of file Paths.cmake
