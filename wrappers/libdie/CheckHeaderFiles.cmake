#
# A CMake module which checks if header files can be included (are present in
#   the system) and if yes locates the directories contaning these files and
#   adds them to the list of include directories used by the compiler.
#
# File:      CheckHeaderFiles.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-02-23
# Date:      Last Update 2012-02-23
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
# Checks if header files can be included (are present in the system) and if yes
#   locates the directories contaning these files and adds them to the list of
#   include directories used by the compiler.
#
# CHECK_HEADER_FILES(<headers> [REQUIRED | <variable>] PATHS <paths>)
#
# If REQUIRED is used, then an error is issued if some of the headers are not
#   found. If REQUIRED is not used, then an output variable must be specified
#   where the result will be written. If all headers are found, this variable
#   will be set to 1, if some of the headers are not found, it will be set to
#   an empty string (i.e. false value). In both cases a list of include paths
#   might be specified which will be searched to locate the header files. The
#   directories where the header files are located will also be automatically
#   added to a list of include directories used by the compiler.
#
MACRO(CHECK_HEADER_FILES)
  PARSE_ARGUMENTS(HEADER_FILES "PATHS" "REQUIRED" ${ARGN})
  # Extract the list of headers we need to check from the default arguments
  if (HEADER_FILES_REQUIRED)
    # Nothing to extract here, all the arguments are the headers to be checked
    set (HEADER_FILES ${HEADER_FILES_DEFAULT_ARGS})
  else (HEADER_FILES_REQUIRED)
    # The last argument is the name of the output variable, need to extract it
    list(LENGTH HEADER_FILES_DEFAULT_ARGS DEFAULT_ARGS_LENGTH)
    math(EXPR LAST_ARG_NUM "${DEFAULT_ARGS_LENGTH}-1")
    list(GET HEADER_FILES_DEFAULT_ARGS ${LAST_ARG_NUM} VARIABLE)
    list(REMOVE_AT HEADER_FILES_DEFAULT_ARGS ${LAST_ARG_NUM})
    set (HEADER_FILES ${HEADER_FILES_DEFAULT_ARGS})
  endif (HEADER_FILES_REQUIRED)

  # C header files can be compiled using a C++ compiler, so prefer C++ over C
  if (CMAKE_CXX_COMPILER_WORKS)
    include(CheckIncludeFileCXX)
  else (CMAKE_CXX_COMPILER_WORKS)
    include(CheckIncludeFile)
  endif (CMAKE_CXX_COMPILER_WORKS)

  # Make sure we do not use some obsolete data from the previous runs
  set(NOT_FOUND_HEADER_FILES)
  set(ADDITIONAL_INCLUDE_DIRS)
  # Set the additional directories searched when checking the header files
  set(CMAKE_REQUIRED_INCLUDES ${HEADER_FILES_PATHS})

  # Notify that we started the header file checking
  if (HEADER_FILES_REQUIRED)
    message(STATUS "Looking for required header files")
  else (HEADER_FILES_REQUIRED)
    message(STATUS "Looking for header files")
  endif (HEADER_FILES_REQUIRED)

  # Check all headers specified by the user
  foreach(HEADER_FILE ${HEADER_FILES})
    # Try to compile a source code contaning one of the headers we check
    if (CMAKE_CXX_COMPILER_WORKS)
      CHECK_INCLUDE_FILE_CXX(${HEADER_FILE} HAVE_HEADER)
    else (CMAKE_CXX_COMPILER_WORKS)
      CHECK_INCLUDE_FILE(${HEADER_FILE} HAVE_HEADER)
    endif (CMAKE_CXX_COMPILER_WORKS)

    # If the source file compiled successfully, the we have the header
    if (HAVE_HEADER)
      # Find the directory where is the header file located and store it
      find_path(HEADER_DIR ${HEADER_FILE} PATHS ${CMAKE_REQUIRED_INCLUDES})
      set(ADDITIONAL_INCLUDE_DIRS ${ADDITIONAL_INCLUDE_DIRS} ${HEADER_DIR})
      unset(HEADER_DIR CACHE)
    else (HAVE_HEADER)
      # We do not have this header file, remember the header file for later
      set (NOT_FOUND_HEADER_FILES ${NOT_FOUND_HEADER_FILES} ${HEADER_FILE})
    endif (HAVE_HEADER)

    # Reuse the variable in the next iteration (remove it from cache)
    unset(HAVE_HEADER CACHE)
  endforeach(HEADER_FILE)

  # Notify the user if we found all the headers
  if (NOT_FOUND_HEADER_FILES)
    # Some headers were not found
    if (HEADER_FILES_REQUIRED)
      # The headers are required to build the project, issue an error
      set(ERROR_MESSAGE
        "The following header files are required, but were not found:\n")
      foreach (HEADER_FILE ${NOT_FOUND_HEADER_FILES})
        set(ERROR_MESSAGE "${ERROR_MESSAGE}  ${HEADER_FILE}\n")
      endforeach(HEADER_FILE)
      message(FATAL_ERROR ${ERROR_MESSAGE})
    else (HEADER_FILES_REQUIRED)
      # The headers are not required, let the user do what he want
      message(STATUS "Looking for header files - some not found")
      set(STATUS_MESSAGE "The following header files were not found:")
      foreach (HEADER_FILE ${NOT_FOUND_HEADER_FILES})
        set(STATUS_MESSAGE "${STATUS_MESSAGE}\n     ${HEADER_FILE}")
      endforeach(HEADER_FILE)
      message(STATUS ${STATUS_MESSAGE})
      # Notify the user that some headers were not found
      set(${VARIABLE} "" CACHE INTERNAL "Have header files ${VARIABLE}")
    endif (HEADER_FILES_REQUIRED)
  else (NOT_FOUND_HEADER_FILES)
    # All headers were found
    if (HEADER_FILES_REQUIRED)
      message(STATUS "Looking for required header files - all found")
    else (HEADER_FILES_REQUIRED)
      message(STATUS "Looking for header files - all found")
    endif (HEADER_FILES_REQUIRED)
    # Better to include the directories containing the checked files
    if (ADDITIONAL_INCLUDE_DIRS)
      # Remove duplicate directories, no need to add them more then once
      list(REMOVE_DUPLICATES ADDITIONAL_INCLUDE_DIRS)
      # Include all directories containing the checked header files
      include_directories(${ADDITIONAL_INCLUDE_DIRS})
      # Print which directories were added, might be usefull to the user
      set(STATUS_MESSAGE
        "The following additional include directories will be used:")
      foreach (HEADER_DIR ${ADDITIONAL_INCLUDE_DIRS})
        set(STATUS_MESSAGE "${STATUS_MESSAGE}\n     ${HEADER_DIR}")
      endforeach(HEADER_DIR)
      message(STATUS ${STATUS_MESSAGE})
    endif (ADDITIONAL_INCLUDE_DIRS)
    # Notify the user that all headers were found
    set(${VARIABLE} 1 CACHE INTERNAL "Have header files ${VARIABLE}")
  endif (NOT_FOUND_HEADER_FILES)
ENDMACRO(CHECK_HEADER_FILES)

# End of file CheckHeaderFiles.cmake
