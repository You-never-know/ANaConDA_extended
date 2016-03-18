#!/bin/bash
#
# Description:
#   A script simplifying debugging the ANaConDA framework, its analysers and
#   the analysed programs with gdb (GNU debugger). This script should NEVER
#   be used directly! It is used by the other scripts to simplify debugging.
# Author:
#   Jan Fiedor
# Version:
#   1.4
# Created:
#   16.03.2015
# Last Update:
#   18.03.2016
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
    if [[ "$line" =~ "Pausing for".+"to attach to process with pid".* ]]; then
      # Get the PID of the process to which we need to attach the debugger
      line_as_array=($line)
      echo "attach "${line_as_array[10]} > commands.gdb
      # We found the section
      break
    fi
  done

  # Skip the next line (no useful info there)
  read line

  # 3rd line contains information about the framework's library
  read line
  echo $line >> commands.gdb

  # Prepare a command for loading additional information obtained later
  echo -e "define loadinfo\n  source `pwd`/info.gdb\nend" >> commands.gdb
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

# Run the debugger in a separate tab or window
if [ "$TERM" == "screen" ]; then
  # We are running in the screen, not terminal
  tmpfile="/tmp/gdb-window-present"
  rm -f $tmpfile

  # Determine if a window named gdb is running
  screen -X msgwait 0
  screen -p "gdb" -X stuff "echo 1 > $tmpfile\n"
  screen -X msgwait 5

  # If no such window is running, create one
  if [ ! -f $tmpfile ]; then
    screen -t "gdb"
  fi

  # Run the debugger in the gdb window
  screen -p "gdb" -X stuff "gdb -x `pwd`/commands.gdb\n"
else
  # Assume we are running is a normal terminal
  konsole --new-tab -e "gdb -x `pwd`/commands.gdb"
fi

# Extract additional information that may be useful to the GNU debugger (gdb)
if [ "$DEBUG_MODE" == "framework" ]; then
  # Check if the ANaConDA framework would not provide additional information
  while read line; do
    if [[ "$line" =~ "Settings" ]]; then
      # End of additional information about the libraries used
      break
    elif [[ "$line" =~ "add-symbol-file".* ]]; then
      # Additional information about the libraries used
      line_as_array=($line)
      # Include only information about libraries GDB is able to locate
      if [ -f ${line_as_array[1]} ]; then
        echo $line >> info.gdb
      fi
    fi
  done
fi

# Discard all the remaining output from the framework, analyser or program
cat >/dev/null

# Clean the temporary files
rm -f commands.gdb info.gdb

# End of script
