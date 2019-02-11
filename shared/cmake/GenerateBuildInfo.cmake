#
# Copyright (C) 2019 Jan Fiedor <fiedorjan@centrum.cz>
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
# A CMake module which generates build information.
#
# File:      GenerateBuildInfo.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2019-01-04
# Date:      Last Update 2019-02-11
# Version:   0.3.1
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
# Generates build information in a form of compile definitions (-D flags) that
#   will be used when compiling the source code (i.e., all source files).
#
# Available build information (generated definitions):
#   BUILD
#     The date of the build in the YYYYMMDD format.
#   GIT_REVISION_SHORT, GIT_REVISION_LONG
#     SHA1 hashes of the git revision used for the build. The short revision
#     is a minimal unique prefix of the SHA1 hash while the long revision is
#     the full SHA1 hash. These definitions are available only when building
#     from a Git repository!
#   GIT_REVISION_IS_MODIFIED
#     A flag indicating if the git revision used for the build was modified.
#     This definition is available only when building from a Git repository!
#   GIT_REVISION_DESCRIPTION_SHORT, GIT_REVISION_DESCRIPTION_LONG
#     A textual description of the git revision used for the build.
#     Short description format:
#       git <sha1-short>[-dirty]                 (if building from Git)
#       no git                                   (if building without Git)
#     Long description format:
#       [modified] git revision <sha1-long>      (if building from Git)
#       unknown git revision                     (if building without Git)
#
# GENERATE_BUILD_INFO(
#   [PREFIX <prefix>]
#   [FILES <file1> [<file2> ...]])
#
# PREFIX
#   A string prepended before definition names, i.e., use <prefix>_<definition>
#   instead of the default <definition> when naming the definitions.
# FILES
#   A list of source files to apply to definitions on (instead of applying them
#   on all source files).
#
MACRO(GENERATE_BUILD_INFO)
  # Get the definition name prefix and files to apply the definitions on
  PARSE_ARGUMENTS(DEFINITION "PREFIX;FILES" "" ${ARGN})

  # Generate a number identifying the build
  string(TIMESTAMP BUILD_DATE "%Y%m%d")

  # By default, use the default bash
  set(BASH "bash")

  # On Windows, use bash from Cygwin if available
  if (WIN32)
    if (EXISTS "$ENV{CYGWIN_HOME}/bin/bash.exe")
      set(BASH "$ENV{CYGWIN_HOME}/bin/bash.exe")
    endif (EXISTS "$ENV{CYGWIN_HOME}/bin/bash.exe")
  endif (WIN32)

  # Get the SHA1 hash of the git revision used for the build
  execute_process(COMMAND ${BASH} "-c" "git rev-parse --short HEAD"
    OUTPUT_VARIABLE GIT_REVISION_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${BASH} "-c" "git rev-parse HEAD"
    OUTPUT_VARIABLE GIT_REVISION_LONG
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Determine if the revision used for the build is modified
  execute_process(COMMAND ${BASH} "-c" "git status --porcelain | grep -v \"^??\""
    OUTPUT_VARIABLE GIT_REVISION_IS_MODIFIED
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # A string prepended before definition names
  if (DEFINITION_PREFIX)
    set(PREFIX "${DEFINITION_PREFIX}_")
  else (DEFINITION_PREFIX)
    set(PREFIX "")
  endif (DEFINITION_PREFIX)

  # Add all available build information to a list
  if (BUILD_DATE)
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}BUILD="${BUILD_DATE}")
  endif (BUILD_DATE)

  if (GIT_REVISION_SHORT)
    # Git revision information are available
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}GIT_REVISION_SHORT="${GIT_REVISION_SHORT}")

    if (GIT_REVISION_IS_MODIFIED)
      # Some files from the revision are modified, flag the revision as dirty
      list(APPEND BUILD_VERSION_INFO
        ${PREFIX}GIT_REVISION_DESCRIPTION_SHORT="git ${GIT_REVISION_SHORT}-dirty")
    else (GIT_REVISION_IS_MODIFIED)
      # All files from the revision are unchanged
      list(APPEND BUILD_VERSION_INFO
        ${PREFIX}GIT_REVISION_DESCRIPTION_SHORT="git ${GIT_REVISION_SHORT}")
    endif (GIT_REVISION_IS_MODIFIED)
  else (GIT_REVISION_SHORT)
    # Git revision information are NOT available
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}GIT_REVISION_DESCRIPTION_SHORT="no git")
  endif (GIT_REVISION_SHORT)

  if (GIT_REVISION_LONG)
    # Git revision information are available
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}GIT_REVISION_LONG="${GIT_REVISION_LONG}")

    if (GIT_REVISION_IS_MODIFIED)
      # Some files from the revision are modified, flag the revision as modified
      list(APPEND BUILD_VERSION_INFO
        ${PREFIX}GIT_REVISION_DESCRIPTION_LONG="modified git revision ${GIT_REVISION_LONG}")
    else (GIT_REVISION_IS_MODIFIED)
      # All files from the revision are unchanged
      list(APPEND BUILD_VERSION_INFO
        ${PREFIX}GIT_REVISION_DESCRIPTION_LONG="git revision ${GIT_REVISION_LONG}")
    endif (GIT_REVISION_IS_MODIFIED)
  else (GIT_REVISION_LONG)
    # Git revision information are NOT available
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}GIT_REVISION_DESCRIPTION_LONG="unknown git revision")
  endif (GIT_REVISION_LONG)

  if (GIT_REVISION_IS_MODIFIED)
    list(APPEND BUILD_VERSION_INFO
      ${PREFIX}GIT_REVISION_IS_MODIFIED=1)
  endif (GIT_REVISION_IS_MODIFIED)

  if (DEFINITION_FILES)
    # Use the definitions only when compiling the given source files
    foreach (FILE ${DEFINITION_FILES})
      set_source_files_properties(${FILE} PROPERTIES COMPILE_DEFINITIONS
        "${BUILD_VERSION_INFO}")
    endforeach (FILE ${DEFINITION_FILES})
  else (DEFINITION_FILES)
    # Use the definitions when compiling any source file
    foreach (DEFINE ${BUILD_VERSION_INFO})
      add_definitions(-D${DEFINE})
    endforeach (DEFINE ${BUILD_VERSION_INFO})
  endif (DEFINITION_FILES)
ENDMACRO(GENERATE_BUILD_INFO)

# End of file GenerateBuildInfo.cmake
