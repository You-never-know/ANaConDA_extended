#!/bin/bash
#
# Description:
#   Functions for preparing executions of programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   1.0
# Created:
#   12.11.2013
# Last Update:
#   12.11.2013
#

# Settings section
# ----------------

# Directory containing information about analysers
ANALYSERS_DIR="./etc/anaconda/tools/analysers"

# Array containing information about analysers
declare -a ANALYSERS

# Directory containing information about programs
PROGRAMS_DIR="./etc/anaconda/tools/programs"

# Array containing information about programs
declare -a PROGRAMS

# Number of threads the executed programs should use
NUMBER_OF_THREADS=`cat /proc/cpuinfo | grep processor | wc -l`

# Functions section
# -----------------

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
  for file in `find $ANALYSERS_DIR -mindepth 1 -maxdepth 1 -type f`; do
    source $file
  done
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
  for file in `find $PROGRAMS_DIR -mindepth 1 -maxdepth 1 -type f`; do
    source $file
  done
}

# End of script
