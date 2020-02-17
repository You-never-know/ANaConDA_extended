#!/bin/bash

#
# Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
# Description:
#   Functions for preparing executions of programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   2.0
# Created:
#   12.11.2013
# Last Update:
#   17.02.2020
#

source utils.sh

# Settings section
# ----------------

# Directory containing information about analysers
ANALYSERS_DIR="analysers"

# Arrays containing information about analysers
declare -a ANALYSERS_PATHS
declare -a ANALYSERS_ARGUMENTS

# Directory containing information about programs
PROGRAMS_DIR="programs"

# Arrays containing information about programs
declare -a PROGRAMS_PATHS
declare -a PROGRAMS_ARGUMENTS

# Number of cores available on the target system
NUMBER_OF_CORES=`cat /proc/cpuinfo | grep processor | wc -l`

# Set the shared libraries extension and PIN launcher program
if [ `uname -o` == "Cygwin" ]; then
  # Windows uses dynamic libraries
  SHARED_LIBRARY_EXTENSION=.dll
  # C-based launcher works, prefer it
  PIN_LAUNCHER=pin.exe
else
  # Linux uses shared objects
  SHARED_LIBRARY_EXTENSION=.so
  # C-based launcher does not set paths correctly, use old launch script
  PIN_LAUNCHER=pin.sh
fi

# Functions section
# -----------------

#
# Description:
#   Sources a given file.
# Parameters:
#   [STRING] A path to a file which should be sourced.
# Output:
#   None
# Return:
#   Nothing
#
source_file()
{
  source $1
}

#
# Description:
#   Gets a unique number identifying an analyser
# Parameters:
#   [STRING] A name (alias) used to identify the analyser.
#   [STRING] A name of the variable to which the number should be stored.
# Output:
#   None
# Return:
#   Nothing
#
get_analyser_id()
{
  get_id "$1" "$2"
}

#
# Description:
#   Registers an analyser.
# Parameters:
#   [STRING] A name (alias) used to identify the analyser.
#   [PATH]   A path to the shared library of the analyser.
#   [LIST]   A list of arguments passed to the analyser.
# Output:
#   None
# Return:
#   Nothing
#
register_analyser()
{
  # Helper variables
  local analyser_id

  # Get the number uniquely identifying the analyser
  get_analyser_id "$1" analyser_id

  # Register the path to the analyser
  ANALYSERS_PATHS[${analyser_id}]="$2"

  # All arguments after the first two are the analyser's arguments
  shift 2
  # Store the analyser's arguments in a dynamically named array variable
  declare -g -a "ANALYSER_${analyser_id}_ARGS"='("$@")'

  # Register the name of the array containing the analyser's arguments (values
  # of bash associative arrays cannot be lists so we can't store the arguments
  # directly, however, we can store the name of a variable containing them and
  # dereference its name later to get the arguments when we need them)
  ANALYSERS_ARGUMENTS[${analyser_id}]="ANALYSER_${analyser_id}_ARGS[@]"
}

#
# Description:
#   Loads analysers.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
load_analysers()
{
  # Import the information about analysers
  process_config_dir "$ANALYSERS_DIR" source_file
}

#
# Description:
#   Setups an analyser. Sets the following variables:
#   - ANALYSER [STRING]
#     A name (alias) used to identify the analyser.
#   - ANALYSER_NAME [STRING]
#     A name of the analyser. A name of the analyser's shared object file.
#   - ANALYSER_PATH [STRING]
#     A path to the analyser. A path to the analyser's shared object file.
#   - ANALYSER_ARGUMENTS [LIST]
#     A list of arguments passed to the analyser. A list containing arguments
#     to be used when executing the analyser.
# Parameters:
#   [STRING] A name (alias) used to identify the analyser.
# Output:
#   An error message if setting up the analyser fails.
# Return:
#   Nothing
#
setup_analyser()
{
  # Helper variables
  local analyser_id

  # Get the name (alias) used to identify the analyser
  ANALYSER=$1

  # Check if the name (alias) is valid
  if [ -z "$ANALYSER" ]; then
    terminate "no analyser specified."
  fi

  # Load the registered analysers
  load_analysers

  # Get the path to the analyser's shared library
  get_analyser_id "$ANALYSER" analyser_id
  ANALYSER_PATH=${ANALYSERS_PATHS[${analyser_id}]}

  # Check if the analyser is registered
  if [ -z "$ANALYSER_PATH" ]; then
    terminate "analyser $ANALYSER not found."
  fi

  # Check if the path to the analyser's shared library is valid
  if [ ! -f "$ANALYSER_PATH" ]; then
    if [ ! -f "$ANALYSER_PATH.so" ]; then
      terminate "analyser's file $ANALYSER_PATH not found."
    fi
  fi

  # Get the analyser's arguments that were stored during its registration (the
  # associative array contains the name of the variable holding the arguments,
  # we need to dereference it to get the arguments themselves)
  ANALYSER_ARGUMENTS=("${!ANALYSERS_ARGUMENTS[${analyser_id}]}")
  # Get the name of the analyser
  ANALYSER_NAME=`basename $ANALYSER_PATH`
}

#
# Description:
#   Gets a unique number identifying a program.
# Parameters:
#   [STRING] A name (alias) used to identify the program.
#   [STRING] A name of the variable to which the number should be stored.
# Output:
#   None
# Return:
#   Nothing
#
get_program_id()
{
  get_id "$1" "$2"
}

#
# Description:
#   Registers a program.
# Parameters:
#   [STRING] A name (alias) used to identify the program.
#   [PATH]   A path to the executable of the program.
#   [LIST]   A list of arguments passed to the program.
# Output:
#   None
# Return:
#   Nothing
#
register_program()
{
  # Helper variables
  local program_id

  # Get the number uniquely identifying the program
  get_program_id "$1" program_id

  # Register the path to the program
  PROGRAMS_PATHS[${program_id}]="$2"

  # All arguments after the first two are the analyser's arguments
  shift 2
  # Store the analyser's arguments in a dynamically named array variable
  declare -g -a "PROGRAM_${program_id}_ARGS"='("$@")'

  # Register the name of the array containing the program's arguments (values
  # of bash associative arrays cannot be lists so we can't store the arguments
  # directly, however, we can store the name of a variable containing them and
  # dereference its name later to get the arguments when we need them)
  PROGRAMS_ARGUMENTS[${program_id}]="PROGRAM_${program_id}_ARGS[@]"
}

#
# Description:
#   Loads programs.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
load_programs()
{
  # Import the information about programs
  process_config_dir "$PROGRAMS_DIR" source_file
}

#
# Description:
#   Setups a program. Sets the following variables:
#   - PROGRAM [STRING]
#     A name (alias) used to identify the program.
#   - PROGRAM_NAME [STRING]
#     A name of the program. A name of the program's executable.
#   - PROGRAM_PATH [STRING]
#     A path to the program. A (full) path to the program's executable.
#   - PROGRAM_ARGUMENTS [LIST]
#     A list of arguments passed to the program. A list containing arguments
#     to be used when executing the program.
# Parameters:
#   [STRING] A name (alias) used to identify the program or a path to the
#            executable of the program.
#   [LIST]   A list of arguments passed to the program (only used when a
#            path to the executable of the program is given).
# Output:
#   An error message if setting up the program fails.
# Return:
#   Nothing
#
setup_program()
{
  # Helper variables
  local program_id

  # First assume that a name (alias) used to identify the program was given
  PROGRAM=$1

  # Check if the name (alias) is valid
  if [ -z "$PROGRAM" ]; then
    terminate "no program specified."
  fi

  # Load the registered programs
  load_programs

  # Get the path to the executable of the program
  get_program_id "$PROGRAM" program_id
  PROGRAM_PATH=${PROGRAMS_PATHS[${program_id}]}

  # Check if the program is registered (if a program with the alias exist)
  if [ -z "$PROGRAM_PATH" ]; then
    # Program not registered, check if it is a path to a valid executable
    PROGRAM_PATH=$PROGRAM

    if [ ! -f "$PROGRAM_PATH" ]; then
      # Not a path to an executable, may be only a name of the executable
      PROGRAM_PATH=`which $PROGRAM`

      if [ $? -ne 0 ]; then
        # No executable with this name was found in directories from PATH
        terminate "program $PROGRAM not found."
      fi
    fi

    # A path to an executable (or its name) was specified, not its alias
    PROGRAM="no-alias"
    # All arguments after the first one are program arguments
    shift
    # Use the arguments given as a parameter to this function
    PROGRAM_ARGUMENTS=("$@")
  else
    # Program registered, check if the path to its executable is valid
    if [ ! -f "$PROGRAM_PATH" ]; then
      # Not a valid path to an executable, but may be its name instead
      PROGRAM_PATH=`which $PROGRAM_PATH`

      if [ $? -ne 0 ]; then
        # No executable with this name found in directories from PATH
        terminate "programs's executable $PROGRAM_PATH not found."
      fi
    fi

    # Get the program's arguments that were stored during its registration (the
    # associative array contains the name of the variable holding the arguments,
    # we need to dereference it to get the arguments themselves)
    PROGRAM_ARGUMENTS=("${!PROGRAMS_ARGUMENTS[${program_id}]}")
  fi

  # Make sure the path to the program's executable is a full path
  PROGRAM_PATH=`readlink -f $PROGRAM_PATH`
  # Extract the name of the program's executable from its path
  PROGRAM_NAME=`basename $PROGRAM_PATH`
}

#
# Description:
#   Setups the environment.
# Parameters:
#   None
# Output:
#   An error message if setting up the environment fails.
# Return:
#   Nothing
#
setup_environment()
{
  # Setup the PIN framework
  if [ -z "$PIN_HOME" ]; then
    terminate "cannot find PIN, set the PIN_HOME variable to point to the installation directory of PIN."
  else
    export LD_LIBRARY_PATH="$PIN_HOME/ia32/runtime/cpplibs:$PIN_HOME/intel64/runtime/cpplibs:$LD_LIBRARY_PATH"
  fi

  if [ `uname -o` != "Cygwin" ]; then
    # Prefer the version of GCC libraries used to compile ANaConDA
    if [ ! -z "$GCC_HOME" ]; then
      switch_gcc $GCC_HOME
    else
      switch_gcc "/usr"
    fi
  fi

  # Prefer the version of libdwarf used to compile ANaConDA
  if [ ! -z "$LIBDWARF_HOME" ]; then
    export LD_LIBRARY_PATH="$LIBDWARF_HOME/libdwarf:$LD_LIBRARY_PATH"
  fi

  # Prefer the version of Boost libraries used to compile ANaConDA
  if [ ! -z "$BOOST_ROOT" ]; then
    export LD_LIBRARY_PATH="$BOOST_ROOT/lib:$LD_LIBRARY_PATH"
  fi
}

#
# Description:
#   Setups the PIN framework. Sets the following variables:
#   - PIN_TARGET_LONG [STRING]
#     A version of the PIN framework to be used to analyse a given program. May
#     be either 'intel64' (for 64-bit programs) or 'ia32' (for 32-bit programs).
#   - PIN_FLAGS [STRING]
#     Command line switches used when executing the PIN framework.
# Parameters:
#   [PATH] A path to the executable of the program to analyse.
# Output:
#   An error message if setting up the PIN framework fails.
# Return:
#   Nothing
#
setup_pin()
{
  # Helper variables
  local program_path=$1

  # Determine the version of the program to analyse (32-bit/64-bit)
  if [ "$HOST_OS" == "windows" ]; then
    # Determine which version of PIN and ANaConDA will be needed (32-bit/64-bit)
    local pe_info=`file $program_path | sed -e "s/.*: \(PE32[+]*\).*/\1/"`

    if [ "$pe_info" == "PE32+" ]; then
      PIN_TARGET_LONG=intel64
    elif [ "$pe_info" == "PE32" ]; then
      PIN_TARGET_LONG=ia32
    else
      terminate "Cannot determine if the program executable $program_path is 32-bit or 64-bit."
    fi
  else
    # Print the content of the AUXV structure. This structure holds information
    # about the version of the executable (32-bit/64-bit). As this command also
    # prints the AUXV information for the shell and the ldd command, we need to
    # extract only the last part that belongs to the program to be executed.
    local arch_info=(`LD_SHOW_AUXV=1 ldd $program_path | grep AT_PLATFORM | tail -1`)

    # Extract the information about the version from the output
    local arch=${arch_info[1]}

    # Determine which version of PIN and ANaConDA will be needed (32-bit/64-bit)
    case "$arch" in
      "x86_64"|"amd64"|"x64")
        PIN_TARGET_LONG=intel64
        ;;
      "x86"|"i686"|"i386")
        PIN_TARGET_LONG=ia32
        ;;
      *)
        terminate "Cannot determine if the program executable $program_path is 32-bit or 64-bit."
        ;;
    esac
  fi

  # Determine the command line switches used when executing the PIN framework
  if [ "$HOST_OS" == "linux" ]; then
    # Get the full version of the Linux kernel we are running
    local kernel_version=`uname -r | sed "s/^\([0-9.]*\).*$/\1/"`
    local kernel_version_parts=( ${kernel_version//./ } 0 0 0 0 )

    # PIN does not support kernel 4.0 and newer yet
    if [ ${kernel_version:0:1} -ge 4 ]; then
      # This undocumented switch will disable the kernel version check
      PIN_FLAGS=-ifeellucky
    # PIN aborts with the 'unexpected AUX VEC type 26' error on kernel 3.10+
    elif [ ${kernel_version_parts[0]} -eq 3 ] \
      && [ ${kernel_version_parts[1]} -ge 10 ]; then
      # This undocumented switch will suppress the error
      PIN_FLAGS=-ifeellucky
    fi
  fi
}

# End of script
