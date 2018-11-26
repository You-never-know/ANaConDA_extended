#!/bin/bash

#
# Copyright (C) 2013-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
#   1.7
# Created:
#   12.11.2013
# Last Update:
#   16.03.2016
#

source utils.sh

# Settings section
# ----------------

# Directory containing information about analysers
ANALYSERS_DIR="analysers"

# Array containing information about analysers
declare -a ANALYSERS

# Directory containing information about programs
PROGRAMS_DIR="programs"

# Array containing information about programs
declare -a PROGRAMS
declare -a PARAMETERS

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
#   [STRING] A path to the analyser.
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
  ANALYSERS[$analyser_id]="$2"
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
#   - ANALYSER_COMMAND [STRING]
#     A path to the analyser together with its arguments. A string containing
#     a path to the analyser's shared object file together with its arguments.
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

  # Get the path to the analyser together with its arguments
  get_analyser_id "$ANALYSER" analyser_id
  ANALYSER_COMMAND=${ANALYSERS[$analyser_id]}

  # Check if the analyser is registered
  if [ -z "$ANALYSER_COMMAND" ]; then
    terminate "analyser $ANALYSER not found."
  fi

  # Get the path to the analyser (without the parameters)
  local analyser_path_with_args=($ANALYSER_COMMAND)
  ANALYSER_PATH=${analyser_path_with_args[0]}

  # Check if the path is valid
  if [ ! -f "$ANALYSER_PATH" ]; then
    if [ ! -f "$ANALYSER_PATH.so" ]; then
      terminate "analyser's file $ANALYSER_PATH not found."
    fi
  fi

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
#   [STRING] A list of parameters passed to the program.
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

  # Register the full command used to execute the program
  PROGRAMS[$program_id]="$2"
  PARAMETERS[$program_id]="$3"
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
#   - PROGRAM_COMMAND [STRING]
#     A path to the program together with its arguments. A string containing
#     a path to the program's executable together with its arguments.
# Parameters:
#   [STRING] A name (alias) used to identify the program or a path to the
#            executable of the program.
#   [STRING] A list of parameters passed to the program (only used when a
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

  # Get a path to the executable of a program identified by the alias given
  get_program_id "$PROGRAM" program_id
  PROGRAM_PATH=${PROGRAMS[$program_id]}

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
    # The second parameter is a list of parameters given to the program
    PROGRAM_COMMAND="$PROGRAM_PATH $2"
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

    # The first parameter is an alias, construct its full command
    PROGRAM_COMMAND="$PROGRAM_PATH ${PARAMETERS[$program_id]}"
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

  # Prefer the version of libdwarf used to compile ANaConDA
  if [ ! -z "$LIBDWARF_HOME" ]; then
    export LD_LIBRARY_PATH="$LIBDWARF_HOME/libdwarf:$LD_LIBRARY_PATH"
  fi

  # Prefer the version of Boost libraries used to compile ANaConDA
  if [ ! -z "$BOOST_ROOT" ]; then
    export LD_LIBRARY_PATH="$BOOST_ROOT/lib:$LD_LIBRARY_PATH"
  fi

  # Skip setting up GCC on Windows as we do not use it there
  if [ `uname -o` == "Cygwin" ]; then
    return
  fi

  # Prefer the version of GCC libraries used to compile ANaConDA
  if [ ! -z "$GCC_HOME" ]; then
    switch_gcc $GCC_HOME
  else
    switch_gcc "/usr"
  fi
}

# End of script
