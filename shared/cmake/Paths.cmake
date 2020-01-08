#
# Copyright (C) 2014-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
# A CMake module which corrects paths when building in mixed environment.
#
# File:      Paths.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2014-10-08
# Date:      Last Update 2019-01-21
# Version:   0.2.1
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

      string(REGEX REPLACE "([^ ]*)/cygdrive/([a-zA-Z])([^ ]*)" "\\1\\2:\\3" PATH ${PATH})

      if (${VAR} MATCHES "^ENV{(.*)}$")
        set(ENV{${CMAKE_MATCH_1}} ${PATH})
      else (${VAR} MATCHES "^ENV{(.*)}$")
        set(${VAR} ${PATH})
      endif (${VAR} MATCHES "^ENV{(.*)}$")
    endif (DEFINED ${VAR} AND NOT "${VAR}" STREQUAL "")
  endforeach(VAR ${ARGN})
ENDMACRO(CORRECT_PATHS)

# End of file Paths.cmake
