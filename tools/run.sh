#!/bin/bash

#
# Copyright (C) 2013-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
#   2.6.9
# Created:
#   14.10.2013
# Last Update:
#   25.01.2019
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

# Program section
# ---------------

# Default values for optional parameters
RUN_TYPE=anaconda
TIME_CMD=
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
        TIME_CMD=/usr/bin/time
      else
        TIME_CMD=time
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
# Preserve quoting of program parameters
for param in "$@"; do
  PROGRAM_PARAMETERS="$PROGRAM_PARAMETERS \"$param\""
done

# Determine the number of threads
if [ -z "$THREADS" ]; then
  # Try to utilize all the processors
  THREADS=$NUMBER_OF_CORES
fi

# Prepare the program (may utilise the THREADS information)
setup_program "$PROGRAM" "$PROGRAM_PARAMETERS"

# This information is useful for include/exclude filters
export PROGRAM_HOME=`dirname $PROGRAM_PATH`
export PROGRAM_NAME

# Determine the version of the program (32-bit/64-bit)
if [ `uname -o` == "Cygwin" ]; then
  # Determine which version of PIN and ANaConDA will be needed (32-bit/64-bit)
  pe_info=`file $PROGRAM_PATH | sed -e "s/.*: \(PE32[+]*\).*/\1/"`

  if [ "$pe_info" == "PE32+" ]; then
    PIN_TARGET_LONG=intel64
  elif [ "$pe_info" == "PE32" ]; then
    PIN_TARGET_LONG=ia32
  elif [ -z "$pe_format" ]; then
    terminate "Cannot determine if the program executable $PROGRAM_PATH is 32-bit or 64-bit."
  else
    terminate "Unsupported executable of type $arch."
  fi
else
  # Print the content of the AUXV structure. This structure contains information
  # about the version of the executable (32/64-bit). Note that this command also
  # prints the AUXV information for the shell and the ldd command, so we need to
  # take only the last information which belong the the program to be executed
  arch_info=(`LD_SHOW_AUXV=1 ldd $PROGRAM_PATH | grep AT_PLATFORM | tail -1`)

  # Extract the information about the version from the output
  arch=${arch_info[1]}

  # Determine which version of PIN and ANaConDA will be needed (32-bit/64-bit)
  case "$arch" in
    "x86_64"|"amd64"|"x64")
      PIN_TARGET_LONG=intel64
      ;;
    "x86"|"i686"|"i386")
      PIN_TARGET_LONG=ia32
      ;;
    *)
      terminate "Cannot determine if the program executable $PROGRAM_PATH is 32-bit or 64-bit."
      ;;
  esac
fi

# Prepare the analyser (may utilise the PIN_TARGET_LONG information)
if [ "$RUN_TYPE" != "native" ]; then
  setup_analyser "$ANALYSER"
fi

# Prepare the environment
setup_environment

# Setup ANaConDA configuration
ANACONDA_FLAGS=

if [ "$VERBOSE" == "1" ]; then
  ANACONDA_FLAGS="$ANACONDA_FLAGS --show-settings"
fi

if [ -z "$CONFIG_DIR" ]; then
  CONFIG_DIR="$SOURCE_DIR/framework/conf"
fi

if [ ! -d "$CONFIG_DIR" ]; then
  terminate "directory containing ANaConDA configuration '"$CONFIG_DIR"' not found."
fi

# Configure the execution to run in debug mode if requested
case "$DEBUG_MODE" in
  "framework") # Debug the framework
    PINTOOL_DEBUG_STRING="-pause_tool ${WAIT_FOR[$DEBUGGER]}"

    if [ "$DEBUGGER" == "gdb" ]; then
      PIPE_COMMANDS="| tee /dev/tty | gdb.sh"

      # ANaConDA can provide more detailed information to the GDB debugger
      ANACONDA_FLAGS="$ANACONDA_FLAGS --debug framework"
    fi
    ;;
  "analyser") # Debug the analyser
    PINTOOL_DEBUG_STRING="-pause_tool ${WAIT_FOR[$DEBUGGER]}"

    if [ "$DEBUGGER" == "gdb" ]; then
      PIPE_COMMANDS="| tee /dev/tty | gdb.sh"

      # ANaConDA can provide more detailed information to the GDB debugger
      ANACONDA_FLAGS="$ANACONDA_FLAGS --debug analyser"
    fi
    ;;
  "program") # Debug the program being analysed
    PINTOOL_DEBUG_STRING="-appdebug"

    if [ "$DEBUGGER" == "gdb" ]; then
      PIPE_COMMANDS="| tee /dev/tty | gdb.sh --debug program"
    fi
    ;;
  *)
    ;;
esac

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
elif [ `uname -s` == "Linux" ] || [ `uname -o` == "GNU/Linux" ]; then
  # Get the full version of the Linux kernel we are running
  kernel_version=`uname -r | sed "s/^\([0-9.]*\).*$/\1/"`
  kernel_version_parts=( ${kernel_version//./ } 0 0 0 0 )

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

# Prepare the command that will run the program
case "$RUN_TYPE" in
  "anaconda")
    RUN_COMMAND="$TIME_CMD \"$PIN_HOME/$PIN_LAUNCHER\" $PINTOOL_DEBUG_STRING $PIN_FLAGS -t \"$ANACONDA_FRAMEWORK_HOME/lib/$PIN_TARGET_LONG/anaconda-framework\" $ANACONDA_FLAGS --config $CONFIG_DIR -a $ANALYSER_COMMAND -- $PROGRAM_COMMAND $PIPE_COMMANDS"
    ;;
  "pin")
    RUN_COMMAND="$TIME_CMD \"$PIN_HOME/$PIN_LAUNCHER\" $PINTOOL_DEBUG_STRING $PIN_FLAGS -t $ANALYSER_COMMAND -- $PROGRAM_COMMAND"
    ;;
  "native")
    RUN_COMMAND="$TIME_CMD $PROGRAM_COMMAND"
    ;;
  *) # This should not happen, but if does better to be notified
    terminate "unknown run type '"$RUN_TYPE"'."
    ;;
esac

# Run the program
print_verbose "executing command '$RUN_COMMAND'."
# Use eval as the command might contain pipes and other stuff
eval $RUN_COMMAND

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
