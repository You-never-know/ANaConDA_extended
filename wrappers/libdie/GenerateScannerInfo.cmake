#
# A CMake module which generates path and symbol information for Eclipse.
#
# File:      GenerateScannerInfo.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-05-31
# Date:      Last Update 2012-06-01
# Version:   0.1.2
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
  string(REGEX MATCHALL "/D[^ ]+" DEFINES ${FLAGS})
  string(REGEX MATCHALL "/I[^ ]+" INCLUDES ${FLAGS})

  # Some (built-in) symbols are defined automatically by the compiler
  set(BUILTIN_DEFINES "__int8=char" "__int16=short" "__int32=int" "__int64=long"
    "__cplusplus=199711L" "__cdecl=__attribute__((__cdecl__))")

  # Have Visual Studio 2010 installed, assume that compiler version is 1600
  if (DEFINED ENV{VS100COMNTOOLS})
    set(BUILTIN_DEFINES ${BUILTIN_DEFINES} "_MSC_VER=1600")
  endif (DEFINED ENV{VS100COMNTOOLS})

  # Export some built-in symbols so the code analyser in Eclipse knows them
  foreach(DEFINE ${BUILTIN_DEFINES})
    string(REPLACE "=" " " DEFINE ${DEFINE})
    file(APPEND ${FILE} "#define ${DEFINE}\n")
  endforeach(DEFINE)

  # Defined symbols must be preceeded by #define and stored one per line
  foreach(ITEM ${DEFINES})
    string(SUBSTRING  ${ITEM} 2 -1 DEFINE)
    string(REPLACE "=" " " DEFINE ${DEFINE})
    file(APPEND ${FILE} "#define ${DEFINE}\n")
  endforeach(ITEM)

  # This text tells Eclipse that a list of include paths follows
  file(APPEND ${FILE} "#include \"...\" search starts here:\n")
  file(APPEND ${FILE} "#include <...> search starts here:\n")

  # PIN's Nmakefile stores MSVC++ include paths to this variable
  if (DEFINED ENV{INCLUDE})
    foreach(ITEM $ENV{INCLUDE})
      file(APPEND ${FILE} " ${ITEM}\n")
    endforeach(ITEM)
  endif (DEFINED ENV{INCLUDE})

  # Include paths must be stored one per line
  foreach(ITEM ${INCLUDES})
    string(SUBSTRING  ${ITEM} 2 -1 INCLUDE)
    file(APPEND ${FILE} " ${INCLUDE}\n")
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
