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
#   A script simplifying running programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   2.10
# Created:
#   14.10.2013
# Last Update:
#   01.03.2020
#

# Search the folder containing the script for the included scripts
PATH=$PATH:$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Include required scripts
source executions.sh

# Settings section
# ----------------

# Array containing information how long (in seconds) to wait for a debugger
declare -gA WAIT_FOR

# GNU debugger is started (and attached) automatically in a separate tab
WAIT_FOR["gdb"]=5
# Eclipse debugger must be attached manually by the user
WAIT_FOR["eclipse"]=20

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
  $0 [--help] [--run-type { anaconda | pin | native }] [--config <dir>] [--time]
     [--threads <number>] [--verbose] [--profile]
     [--debug { framework | analyser | program }] [--debugger { gdb | eclipse }]
     <analyser> <program> [<program-parameters>]

required arguments:
  <analyser>  A name of the analyser to be used.
  <program>   A name of the program to be analysed. May be its alias or a path
              to the executable of the program. If a path to the executable is
              given, one may also specify the parameters given to it.

optional arguments:
  --help
    Print the script usage.
  --run-type { anaconda | pin | native }
    Execute the program in ANaConDA, PIN or no framework (native run). Default
    is to run the program in ANaConDA.
  --config <dir>
    A path to a directory containing ANaConDA settings. Default is a directory
    containing the default ANaConDA settings (framework/conf).
  --time
    Measure the execution time of the program being analysed.
  --threads
    A number of threads the analysed program should utilize. If not specified,
    the number of cores available will be used as the number for threads. Note
    that this number is just a recommendation and the target program might use
    a different number or no number at all (if the number of threads cannot be
    set at all as the program just always use how many threads are necessary).
    When registering a program, one may use the THREADS variable to access the
    recommended number of threads the program should utilize.
  --verbose
    Print detailed information about what the script is doing.
  --profile
    Profile the program being analysed using the oprofile profiler. You need to
    be able to run the operf and kill commands as a root! If running this script
    as a standard user, you need to add these lines to the /etc/sudoers file:
      <your-account-name> ALL=NOPASSWD: <path-to-operf>
      <your-account-name> ALL=NOPASSWD: <path-to-kill>
    For example, for a user named 'john' and the operf and kill commands stored
    at the standard locations, the lines will look like this:
      john ALL=NOPASSWD: /usr/bin/operf
      john ALL=NOPASSWD: /usr/bin/kill
  --debug { framework | analyser | program }
    Debug the framework, analyser or the program being analysed using the chosen
    debugger.
  --debugger { gdb | eclipse }
    Use the gdb or eclipse debugger to debug the application. If gdb is chosen
    as a debugger, it will be started in a separate console tab, automatically
    attached to the running framework, analyser or program and configured with
    the information provided. If eclipse is chosen, the user must attach the
    eclipse debugger to the framework, analyser or program manually. Default
    debugger is gdb.
"
}

#
# Description:
#   Checks if the oprofile profiler is available.
# Parameters:
#   None
# Return:
#   Nothing
#
check_oprofile()
{
  # Get the version of the oprofile profiler
  OPROFILE_VERSION=`operf --version | grep -E -o "operf: oprofile [0-9.]+" | grep -E -o "[0-9.]+"`

  if [ -z "$OPROFILE_VERSION" ]; then
    terminate "oprofile not found."
  fi

  # Check if we will be able to run oprofile under the root privileges
  if [[ $EUID -ne 0 ]]; then
    # We need to be able to run the operf command as root for system-wide profiling
    OPERF_CMD_CHECK=`sudo -n operf`

    if [ "$OPERF_CMD_CHECK" == "sudo: a password is required" ]; then
      terminate "cannot run oprofile under the root privileges (cannot run operf as a root), run this script as a root or add yourself to the /etc/sudoers (see help for more information)."
    fi

    # We need to send a SIGINT signal to the operf to stop the system-wide profiling
    KILL_CMD_CHECK=`sudo -n kill`

    if [ "$KILL_CMD_CHECK" == "sudo: a password is required" ]; then
      terminate "cannot run oprofile under the root privileges (cannot run kill as a root), run this script as a root or add yourself to the /etc/sudoers (see help for more information)."
    fi
  fi
}

#
# Description:
#   Configures the script to run the analysis or program in debug mode. Sets
#   or updates the following variables:
#   - PIN_FLAGS [LIST]
#     A list of command line switches used when executing the PIN framework.
#   - OUTPUT_PARSER [STRING]
#     A name of the script for extracting information for a chosen debugger.
#   - OUTPUT_PARSER_ARGUMENTS [LIST]
#     A list of arguments passed to the script.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
configure_debug()
{
  case "$DEBUG_MODE" in
    "framework") # Debug the framework
      PIN_FLAGS=("${PIN_FLAGS[@]}" "-pause_tool" "${WAIT_FOR[$DEBUGGER]}")

      if [ "$DEBUGGER" == "gdb" ]; then
        OUTPUT_PARSER="gdb.sh"

        # ANaConDA can provide more detailed information to the GDB debugger
        ANACONDA_FLAGS=("${ANACONDA_FLAGS[@]}" "--debug" "framework")
      fi
      ;;
    "analyser") # Debug the analyser
      PIN_FLAGS=("${PIN_FLAGS[@]}" "-pause_tool" "${WAIT_FOR[$DEBUGGER]}")

      if [ "$DEBUGGER" == "gdb" ]; then
        OUTPUT_PARSER="gdb.sh"

        # ANaConDA can provide more detailed information to the GDB debugger
        ANACONDA_FLAGS=("${ANACONDA_FLAGS[@]}" "--debug" "analyser")
      fi
      ;;
    "program") # Debug the program being analysed
      PIN_FLAGS=("${PIN_FLAGS[@]}" "-appdebug")

      if [ "$DEBUGGER" == "gdb" ]; then
        OUTPUT_PARSER="gdb.sh"
        OUTPUT_PARSER_ARGUMENTS=("${OUTPUT_PARSER_ARGUMENTS[@]}" "--debug" "program")
      fi
      ;;
    *)
      ;;
  esac
}

#
# Description:
#   Runs an analysis of a program using a chosen analyser for the ANaConDA
#   framework. The command to run is created using the following variables:
#   - TIME_COMMAND [STRING]
#     A name or path to a command or program for measuring execution time.
#   - PIN_LAUNCHER_PATH [PATH]
#     A path to the PIN framework's launcher.
#   - PIN_FLAGS [LIST]
#     A list of command line switches used when executing the PIN framework.
#   - ANACONDA_FRAMEWORK_PATH [PATH]
#     A path to the ANaConDA framework's shared library.
#   - ANACONDA_FLAGS [LIST]
#     A list of command line switches used when executing the ANaConDA framework.
#   - CONFIG_DIR [PATH]
#     A path to a directory containing ANaConDA framework's configuration files.
#   - ANALYSER_PATH [PATH]
#     A path to the ANaConDA analyser.
#   - ANALYSER_ARGUMENTS [LIST]
#     A list of arguments passed to the analyser.
#   - PROGRAM_PATH [PATH]
#     A path to the program to analyse.
#   - PROGRAM_ARGUMENTS [LIST]
#     A list of arguments passed to the program.
#   See the `run_command` function for a list of variables that influence how
#   the created command is ran.
# Parameters:
#   None
# Output:
#   The output of the analysis (analyser used), the ANaConDA framework and the
#   program being analysed. If the script is running in verbose mode, the full
#   command executing the analysis is also printed to the output.
# Return:
#   Nothing
#
run_anaconda()
{
  run_command $TIME_COMMAND \
    "$PIN_LAUNCHER_PATH" "${PIN_FLAGS[@]}" \
    -t "$ANACONDA_FRAMEWORK_PATH" "${ANACONDA_FLAGS[@]}" \
    --config "$CONFIG_DIR" \
    -a "$ANALYSER_PATH" "${ANALYSER_ARGUMENTS[@]}" \
    -- "$PROGRAM_PATH" "${PROGRAM_ARGUMENTS[@]}"
}

#
# Description:
#   Runs an analysis of a program using a chosen pintool (plugin for the PIN
#   framework). The command to run is created using the following variables:
#   - TIME_COMMAND [STRING]
#     A name or path to a command or program for measuring execution time.
#   - PIN_LAUNCHER_PATH [PATH]
#     A path to the PIN framework's launcher.
#   - PIN_FLAGS [LIST]
#     A list of command line switches used when executing the PIN framework.
#   - ANALYSER_PATH [PATH]
#     A path to the pintool.
#   - ANALYSER_ARGUMENTS [LIST]
#     A list of arguments passed to the pintool.
#   - PROGRAM_PATH [PATH]
#     A path to the program to analyse.
#   - PROGRAM_ARGUMENTS [LIST]
#     A list of arguments passed to the program.
#   See the `run_command` function for a list of variables that influence how
#   the created command is ran.
# Parameters:
#   None
# Output:
#   The output of the analysis (pintool used), the Intel PIN framework and the
#   program being analysed. If the script is running in verbose mode, the full
#   command executing the analysis is also printed to the output.
# Return:
#   Nothing
#
run_pin()
{
  run_command $TIME_COMMAND \
    "$PIN_LAUNCHER_PATH" "${PIN_FLAGS[@]}" \
    -t "$ANALYSER_PATH" "${ANALYSER_ARGUMENTS[@]}" \
    -- "$PROGRAM_PATH" "${PROGRAM_ARGUMENTS[@]}"
}

#
# Description:
#   Runs a program directly. The command to run is created using the following
#   variables:
#   - TIME_COMMAND [STRING]
#     A name or path to a command or program for measuring execution time.
#   - PROGRAM_PATH [PATH]
#     A path to the program.
#   - PROGRAM_ARGUMENTS [LIST]
#     A list of arguments passed to the program.
#   See the `run_command` function for a list of variables that influence how
#   the created command is ran.
# Parameters:
#   None
# Output:
#   The output of the program. If the script is running in verbose mode, the
#   full command executing the program is also printed to the output.
# Return:
#   Nothing
#
run_program()
{
  run_command $TIME_COMMAND \
    "$PROGRAM_PATH" "${PROGRAM_ARGUMENTS[@]}"
}

#
# Description:
#   Runs a given command. The following variables influence how the command is
#   ran (e.g., if the command should be echoed, its output redirected, etc.):
#   - VERBOSE [INTEGER]
#     An integer flag specifying if the script is running in verbose mode. If
#     the flag value is 1, the command itself will be printed to the output.
#   - OUTPUT_PARSER [STRING]
#     A name of a program or script for processing the output of the command.
#     If the variable is set, a copy of the command's output is redirected to
#     the specified program or script.
#   - OUTPUT_PARSER_ARGUMENTS [LIST]
#     A list of arguments passed to the output parser.
# Parameters:
#   [STRING] A name or path to the command or executable to run.
#   [LIST]   A list of arguments passed to the command or executable.
# Output:
#   The output of the command. If the script is running in verbose mode, the
#   command itself is also printed to the output.
# Return:
#   Nothing
#
run_command()
{
  if [ -z "$OUTPUT_PARSER" ]; then
    # No program or script to redirect the command's output to
    if [ "$VERBOSE" == "1" ]; then
      # Verbose mode, print the command itself to the output
      set -x
      "$@"
      { set +x; } 2>/dev/null
    else
      # Normal mode, do not print the command to the output
      "$@"
    fi
  else
    # Redirect the command's output to a given program or script
    if [ "$VERBOSE" == "1" ]; then
      # Verbose mode, print the command itself to the output
      set -x
      "$@" | tee /dev/tty | "$OUTPUT_PARSER" "${OUTPUT_PARSER_ARGUMENTS[@]}"
      { set +x; } 2>/dev/null
    else
      # Normal mode, do not print the command to the output
      "$@" | tee /dev/tty | "$OUTPUT_PARSER" "${OUTPUT_PARSER_ARGUMENTS[@]}"
    fi
  fi
}

# Program section
# ---------------

# Default values for optional parameters
RUN_TYPE=anaconda
TIME_COMMAND=
VERBOSE=0
PROFILE=0
DEBUG_MODE=
DEBUGGER=gdb

# Initialize environment first, optional parameters might override the values
env_init

# Process optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--run-type")
      if [ -z "$2" ]; then
        terminate "missing run type."
      fi
      if ! [[ "$2" =~ ^anaconda|pin|native$ ]]; then
        terminate "run type must be anaconda, pin or native."
      fi
      RUN_TYPE=$2
      shift
      ;;
    "--config")
      if [ -z "$2" ]; then
        terminate "missing config directory."
      fi
      if [ ! -d "$2" ]; then
        terminate "'"$2"' is not a directory."
      fi
      CONFIG_DIR=$2
      shift
      ;;
    "--time")
      if [ -f /usr/bin/time ]; then
        TIME_COMMAND=/usr/bin/time
      else
        TIME_COMMAND=time
      fi
      ;;
    "--threads")
      if [ -z "$2" ]; then
        terminate "missing number of threads."
      fi
      if ! [[ "$2" =~ ^[0-9]+$ ]]; then
        terminate "number of threads must be a number."
      fi
      THREADS=$2
      shift
      ;;
    "--verbose")
      set_mode verbose
      VERBOSE=1
      ;;
    "--profile")
      check_oprofile
      PROFILE=1
      ;;
    "--debug")
      if [ -z "$2" ]; then
        terminate "missing debug mode."
      fi
      if ! [[ "$2" =~ ^framework|analyser|program$ ]]; then
        terminate "debug mode must be framework, analyser or program."
      fi
      DEBUG_MODE=$2
      shift
      ;;
    "--debugger")
      if [ -z "$2" ]; then
        terminate "missing the debugger."
      fi
      if ! [[ "$2" =~ ^gdb|eclipse$ ]]; then
        terminate "debugger must be gdb or eclipse."
      fi
      DEBUGGER=$2
      shift
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Process positional parameters
ANALYSER=$1
shift
PROGRAM=$1
shift
PROGRAM_ARGUMENTS=("$@")

# Determine the number of threads
if [ -z "$THREADS" ]; then
  # Try to utilize all the processors
  THREADS=$NUMBER_OF_CORES
fi

# Prepare the program (may utilise the THREADS information)
setup_program "$PROGRAM" "${PROGRAM_ARGUMENTS[@]}"

# This information is useful for include/exclude filters
export PROGRAM_HOME=`dirname $PROGRAM_PATH`
export PROGRAM_NAME

# Setup the PIN framework (sets the PIN_TARGET_LONG information)
if [ "$RUN_TYPE" != "native" ]; then
  setup_pin "$PROGRAM_PATH"
fi

# Setup the ANaConDA framework
if [ "$RUN_TYPE" == "anaconda" ]; then
  setup_anaconda
fi

# Prepare the analyser (may utilise the PIN_TARGET_LONG information)
if [ "$RUN_TYPE" != "native" ]; then
  setup_analyser "$ANALYSER"
fi

# Prepare the environment
setup_environment

# Setup ANaConDA configuration
ANACONDA_FLAGS=()

if [ "$VERBOSE" == "1" ]; then
  ANACONDA_FLAGS=("${ANACONDA_FLAGS[@]}" "--show-settings")
fi

if [ -z "$CONFIG_DIR" ]; then
  CONFIG_DIR="$SOURCE_DIR/framework/conf"
fi

if [ ! -d "$CONFIG_DIR" ]; then
  terminate "directory containing ANaConDA configuration '"$CONFIG_DIR"' not found."
fi

# Configure the analysis or program to run in debug mode if requested
if [ ! -z "$DEBUG_MODE" ]; then
  configure_debug
fi

# Remove old log files
rm -f pintool.log

# Setup and start the system-wide profiling before running the program
if [ "$PROFILE" == "1" ]; then
  # Remove old temporary files
  rm -rf operf.out

  # Start the system-wide profiling at the background
  sudo operf --system-wide --vmlinux /usr/lib/modules/`uname -r`/build/vmlinux &> operf.out &

  # Wait for the profiling to start
  while [ ! -f operf.out ]; do
    sleep 0.1
  done

  # After the profiling started, operf.out will contain the command to stop it
  KILL_OPERF=$(cat operf.out | grep -E -o 'kill -SIGINT [0-9]*')
  while [ "$KILL_OPERF" == "" ]; do
    sleep 0.1
    KILL_OPERF=$(cat operf.out | grep -E -o 'kill -SIGINT [0-9]*')
  done
fi

# Operating system-specific configuration
if [ `uname -o` == "Cygwin" ]; then
  # When running in Cygwin, we need to start PIN using the Cygwin path, however,
  # paths to the ANaConDA framework, analyser, and the analysed program must be
  # in a Windows format as PIN will access them using the Windows filesystem
  correct_paths ANACONDA_FRAMEWORK_HOME ANALYSER_COMMAND PROGRAM_COMMAND CONFIG_DIR

  # Add paths to PIN and ANaConDA runtime libraries to PATH
  PATH=$PATH:$ANACONDA_FRAMEWORK_HOME/lib/$PIN_TARGET_LONG:$PIN_HOME/$PIN_TARGET_LONG/bin
fi

# Run the analysis (or program)
case "$RUN_TYPE" in
  "anaconda")
    run_anaconda
    ;;
  "pin")
    run_pin
    ;;
  "native")
    run_program
    ;;
  *) # This should not happen, but if does better to be notified
    terminate "unknown run type '"$RUN_TYPE"'."
    ;;
esac

# Stop the system-wide profiling and process the results
if [ "$PROFILE" == "1" ]; then
  # Send the SIGINT signal to the operf running in the background
  eval "sudo $KILL_OPERF"

  # Process the results
  opreport -l "$PIN_HOME/intel64/bin/pinbin" > pin64.profile
  opreport -l "$PIN_HOME/ia32/bin/pinbin" > pin32.profile
  opreport -l "$PROGRAM_PATH" > $PROGRAM_NAME.profile

  # Remove temporary files
  rm -rf operf.out
fi

# End of script
