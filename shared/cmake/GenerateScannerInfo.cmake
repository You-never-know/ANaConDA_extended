#
# Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
# A CMake module which generates path and symbol information for Eclipse.
#
# File:      GenerateScannerInfo.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-05-31
# Date:      Last Update 2015-06-08
# Version:   0.1.7
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
# Generates path and symbol information for Eclipse from compiler flags.
#
# GENERATE_SCANNER_INFO(<file>
#   [FLAG_VARS <var1> [<var2> ...]]
#   [PATH_VARS <var1> [<var2> ...]])
#
MACRO(GENERATE_SCANNER_INFO FILE)
  # Get the user-specified variables containing the compiler flags
  PARSE_ARGUMENTS(SCANNER "FLAG_VARS;PATH_VARS" "" ${ARGN})

  set(FLAGS "")
  # Concatenate the flags into a single string to process all of them at once
  foreach(ITEM ${SCANNER_FLAG_VARS})
    set(FLAGS "${FLAGS} ${${ITEM}}")
  endforeach(ITEM ${SCANNER_FLAG_VARS})

  # Create the file where the include paths and defined symbols will be written
  file(WRITE ${FILE} "")

  # Find all defined symbols (/D switch) and include paths (/I switch)
  string(REGEX MATCHALL "[ ]/D[^ ]+" DEFINES ${FLAGS})
  string(REGEX MATCHALL "[ ]/I[^ ]+" INCLUDES ${FLAGS})

  # ANSI-Compliant Predefined Macros
  set(BUILTIN_C99_DEFINES "__STDC__")
  # Microsoft-Specific Predefined Macros
  set(BUILTIN_MS_DEFINES "_ATL_VER" "__AVX__" "__AVX2__" "_CHAR_UNSIGNED"
    "__CLR_VER" "__cplusplus_cli" "__cplusplus_winrt" "__COUNTER__"
    "__cplusplus" "_CPPRTTI" "_CPPUNWIND" "_DEBUG" "_DLL" "__FUNCDNAME__"
    "__FUNCSIG__" "__FUNCTION__" "_INTEGRAL_MAX_BITS" "_M_AMD64" "_M_ARM"
    "_M_CEE" "_M_CEE_PURE" "_M_CEE_SAFE" "_M_IX86" "_M_ARM_FP" "_M_IX86_FP"
    "_M_X64" "_MANAGED" "_MFC_VER" "_MSC_BUILD" "_MSC_EXTENSIONS"
    "_MSC_FULL_VER" "_MSC_VER" "__MSVC_RUNTIME_CHECKS" "_MT"
    "_NATIVE_WCHAR_T_DEFINED" "_OPENMP" "_VC_NODEFAULTLIB" "_WCHAR_T_DEFINED"
    "_WIN32" "_WIN64")
  # All Predefined Macros
  set(BUILTIN_DEFINES ${BUILTIN_C99_DEFINES} ${BUILTIN_MS_DEFINES})

  # Determine the source file used to retrieve the values of the defines
  set(COMPILE_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}")
  set(SOURCE_FILE "${COMPILE_DIR}/compiler_defs.cpp")

  # Helper macros which use preprocessor to print the values of the defines
  file(WRITE "${SOURCE_FILE}"
    "#define __STR2__(x) #x\n"
    "#define __STR1__(x) __STR2__(x)\n"
    "#define __PPOUT__(x) \"#define \" #x \" \" __STR1__(x)\n\n")

  # This code prints the values of the specified defines
  foreach(DEFINE ${BUILTIN_DEFINES})
    file(APPEND "${SOURCE_FILE}"
      "#if defined(${DEFINE})\n"
      "  #pragma message(__PPOUT__(${DEFINE}))\n"
      "#endif\n\n")
  endforeach(DEFINE ${BUILTIN_DEFINES})

  # Compile the file so the preprocessor would print the values of the defines
  try_compile(RESULT "${COMPILE_DIR}" "${SOURCE_FILE}"
    OUTPUT_VARIABLE OUTPUT)

  # Extract the lines containing the values of the defines from the output
  string(REGEX MATCHALL "#define[^\n]*\n" BUILTIN_DEFINES ${OUTPUT})

  # Added the built-in defines so the code analyser in Eclipse knows them
  foreach(DEFINE ${BUILTIN_DEFINES})
    string(REPLACE "\n" "" DEFINE ${DEFINE})
    file(APPEND ${FILE} "${DEFINE}")
  endforeach(DEFINE)

  # Special symbols recognised by the compiler, but not defined anywhere
  set(SPECIAL_SYMBOLS "__int8=char" "__int16=short" "__int32=int" "__int64=long"
    "__cdecl=__attribute__((__cdecl__))")

  # Include the special symbols so the code analyser in Eclipse knows them
  foreach(SYMBOL ${SPECIAL_SYMBOLS})
    string(REPLACE "=" " " SYMBOL ${SYMBOL})
    file(APPEND ${FILE} "#define ${SYMBOL}\n")
  endforeach(SYMBOL)

  # Defined symbols must be preceeded by #define and stored one per line
  foreach(ITEM ${DEFINES})
    string(SUBSTRING ${ITEM} 3 -1 DEFINE)
    string(REPLACE "=" " " DEFINE ${DEFINE})
    file(APPEND ${FILE} "#define ${DEFINE}\n")
    # PIN makefiles use /D on Windows, however, CDT output parser looks for -D
    if (WIN32)
      string(REPLACE " " "=" DEFINE ${DEFINE})
      add_definitions(-D${DEFINE})
    endif (WIN32)
  endforeach(ITEM)

  # This text tells Eclipse that a list of include paths follows
  file(APPEND ${FILE} "#include \"...\" search starts here:\n")
  file(APPEND ${FILE} "#include <...> search starts here:\n")

  # PIN's Nmakefile stores MSVC++ include paths to this variable
  if (DEFINED ENV{INCLUDE})
    foreach(ITEM $ENV{INCLUDE})
      file(APPEND ${FILE} " ${ITEM}\n")
      # CDT's VC++ toolchain does not provide this info, scan it from output
      if (WIN32)
        include_directories(${ITEM})
      endif (WIN32)
    endforeach(ITEM)
  endif (DEFINED ENV{INCLUDE})

  # Include paths must be stored one per line
  foreach(ITEM ${INCLUDES})
    string(SUBSTRING ${ITEM} 3 -1 INCLUDE)
    file(APPEND ${FILE} " ${INCLUDE}\n")
    # PIN makefiles use /I on Windows, however, CDT output parser looks for -I
    if (WIN32)
      include_directories(${INCLUDE}) # Uses -I when adding the include paths
    endif (WIN32)
  endforeach(ITEM)

  # Here can be added additional paths not present in the compiler flags
  foreach(VAR ${SCANNER_PATH_VARS})
    foreach(PATH ${${VAR}})
      file(TO_NATIVE_PATH ${PATH} NATIVE_PATH)
      file(APPEND ${FILE} " ${NATIVE_PATH}\n")
    endforeach(PATH ${${VAR}})
  endforeach(VAR ${SCANNER_PATH_VARS})

  # This text tells Eclipse that a list of include paths ends here
  file(APPEND ${FILE} "End of search list.\n")
ENDMACRO(GENERATE_SCANNER_INFO)

# End of file GenerateScannerInfo.cmake
