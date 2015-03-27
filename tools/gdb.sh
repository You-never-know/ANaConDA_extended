#!/bin/bash
#
# Description:
#   A script simplifying debugging the ANaConDA framework, its analysers and
#   the analysed programs with gdb (GNU debugger). This script should NEVER
#   be used directly! It is used by the other scripts to simplify debugging.
# Author:
#   Jan Fiedor
# Version:
#   1.1
# Created:
#   16.03.2015
# Last Update:
#   27.03.2015
#

# Functions section
# -----------------

#
# Description:
#   Prints a script usage.
# Parameters:
#   None
# Output:
#   A description of the script usage.
# Return:
#   Nothing
#
usage()
{
  echo -e "\
usage:
  $0 [--help] [--debug { framework | analyser | program }]

optional arguments:
  --help
    Print the script usage.
  --debug { framework | analyser | program }
    Configure the GNU debugger (gdb) to debug the framework, analyser or the
    program being analysed. Default is the framework.
"
}

# Program section
# ---------------

# Default values for optional parameters
DEBUG_MODE=framework

# Process optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--debug")
      if [ -z "$2" ]; then
        terminate "missing which part will be debugged."
      fi
      if ! [[ "$2" =~ ^framework|analyser|program$ ]]; then
        terminate "can debug only framework, analyser or program."
      fi
      DEBUG_MODE=$2
      shift
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Configure the GNU debugger (gdb) based on what we want to debug
if [ "$DEBUG_MODE" == "framework" ]; then
  # Get to the section containing information about the framework process
  while read line; do
    if [[ "$line" =~ "Pausing to attach to pid"* ]]; then
      # Get the PID of the process to which we need to attach the debugger
      line_as_array=($line)
      echo "attach "${line_as_array[5]} > commands.gdb
      # We found the section
      break
    fi
  done

  # Skip the next line (no useful info there)
  read line

  # 3rd line contains information about the framework's library
  read line
  echo $line >> commands.gdb
elif [ "$DEBUG_MODE" == "program" ]; then
  # Get to the section containing information about the program process 
  while read line; do
    if [[ "$line" =~ "Application stopped until continued from debugger"* ]]; then
      # We found the section
      break
    fi
  done

  # Skip the next line (no useful info there)
  read line

  # 3rd line contains a command to connect to the program we want to debug
  read line
  echo $line > commands.gdb
fi

# Run the debugger in a separate tab
konsole --new-tab -e "gdb -x `pwd`/commands.gdb"

# Discard all the remaining output from the framework, analyser or program
cat >/dev/null

# Clean the temporary files
rm -f commands.gdb

# End of script
