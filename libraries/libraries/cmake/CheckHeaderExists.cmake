#
# Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of libdie.
#
# libdie is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# libdie is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libdie. If not, see <http://www.gnu.org/licenses/>.
#

#
# A CMake module which checks if a header exists.
#
# File:      CheckHeaderExists.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-24
# Date:      Last Update 2012-02-25
# Version:   0.1
#

#
# A helper macro which parses arguments of another macro (for more information
#   see the desctiption at http://www.cmake.org/Wiki/CMakeMacroParseArguments)
#
# PARSE_ARGUMENTS(prefix arg_names options arg1 arg2 ...) 
#
MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
         SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
         SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

#
# Checks if a header exists.
#
# CHECK_HEADER_EXISTS(<header> <variable>
#   [REQUIRED]
#   [PATHS <path1> [<path2> ... ]]
#   [PATH_SUFFIXES <suffix1> [<suffix2> ...]])
#
MACRO(CHECK_HEADER_EXISTS HEADER VARIABLE)
  # Check if required flag set and get the user-specified paths and suffixes 
  PARSE_ARGUMENTS(HEADER "PATHS;PATH_SUFFIXES" "REQUIRED" ${ARGN})

  # Notify that we started looking for the header
  message(STATUS "Looking for ${HEADER}")

  # First search the include directories specified by the user
  find_path(${VARIABLE} NAMES ${HEADER} PATHS ${HEADER_PATHS} NO_DEFAULT_PATH
    PATH_SUFFIXES ${HEADER_PATH_SUFFIXES})
  # If the header was not found, search the default paths
  find_path(${VARIABLE} NAMES ${HEADER})

  # Notify that we finished looking for the header
  if (${VARIABLE})
    # Notify that we found the header (header directory is now in output var)
    message(STATUS "Looking for ${HEADER} - found")
  else (${VARIABLE})
    # Notify that we did not find the header (output var is now set to false)
    message(STATUS "Looking for ${HEADER} - not found")
    # If the header is required, stop the build process (issue a fatal error)
    if (HEADER_REQUIRED)
      message(FATAL_ERROR "Header ${HEADER} is required\n")
    endif (HEADER_REQUIRED)
  endif(${VARIABLE})
ENDMACRO(CHECK_HEADER_EXISTS)

# End of file CheckHeaderExists.cmake
