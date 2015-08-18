#!/bin/bash
#
# Description:
#   Functions for preparing executions of programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   1.4.2
# Created:
#   12.11.2013
# Last Update:
#   18.08.2015
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
#   [STRING] A full command used to execute the program (path to the executable
#            together with parameters).
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
#     A path to the program. A path to the program's executable.
#   - PROGRAM_COMMAND [STRING]
#     A path to the program together with its arguments. A string containing
#     a path to the program's executable together with its arguments.
# Parameters:
#   [STRING] A name (alias) used to identify the program.
# Output:
#   An error message if setting up the program fails.
# Return:
#   Nothing
#
setup_program()
{
  # Helper variables
  local program_id

  # Get the name (alias) used to identify the program
  PROGRAM=$1

  # Check if the name (alias) is valid
  if [ -z "$PROGRAM" ]; then
    terminate "no program specified."
  fi

  # Load the registered programs
  load_programs

  # Get the path to the program together with its arguments
  get_program_id "$PROGRAM" program_id
  PROGRAM_COMMAND=${PROGRAMS[$program_id]}

  # Check if the program is registered
  if [ -z "$PROGRAM_COMMAND" ]; then
    terminate "program $PROGRAM not found."
  fi

  # Get the path to the program (without the parameters)
  local program_path_with_args=($PROGRAM_COMMAND)
  PROGRAM_PATH=${program_path_with_args[0]}

  # Check if the path is valid
  if [ ! -f "$PROGRAM_PATH" ]; then
    terminate "programs's executable $PROGRAM_PATH not found."
  fi

  # Get the name of the program
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
  if [ ! -z "$BOOST_HOME" ]; then
    export LD_LIBRARY_PATH="$BOOST_HOME/lib:$LD_LIBRARY_PATH"
  fi

  # Prefer the version of GCC libraries used to compile ANaConDA
  if [ ! -z "$GCC_HOME" ]; then
    switch_gcc $GCC_HOME
  else
    switch_gcc "/usr"
  fi
}

# End of script
