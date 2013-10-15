#!/bin/bash
#
# Description:
#    A script simplifying running programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   0.1
# Created:
#   14.10.2013
# Last Update:
#   15.10.2013
#

source messages.sh

# Settings section
# ----------------

# Directory containing information about analysers
ANALYSERS_DIR="./etc/anaconda/tools/analysers"

# Directory containing information about programs
PROGRAMS_DIR="./etc/anaconda/tools/programs"

# Number of threads which the programs should use
PREFFERED_NUMBER_OF_THREADS=`cat /proc/cpuinfo | grep processor | wc -l`

# Functions section
# -----------------

#
# Description:
#   Prints a script usage
# Parameters:
#   None
# Return:
#   Nothing
#
usage()
{
  echo -e "\
usage:
  $0 [--help] [--config <dir>] [--run-type { anaconda | pin | native }] [--time]
     [--verbose] [--profile] <analyser> <program>

required arguments:
  <analyser>  A name of the analyser to be used.
  <program>   A name of the program to be analysed.

optional arguments:
  --help
    Print the script usage.
  --config <dir>
    A path to a directory containing ANaConDA settings.
  --run-type { anaconda | pin | native }
    Execute the program in ANaConDA, PIN or no framework (native run). Default
    is to run the program in ANaConDA.
  --time
    Measure the execution time of the program being analysed.
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
    print_error "oprofile not found."
    exit 1
  fi

  # Check if we will be able to run oprofile under the root privileges
  if [[ $EUID -ne 0 ]]; then
    # We need to be able to run the operf command as root for system-wide profiling
    OPERF_CMD_CHECK=`sudo -n operf`

    if [ "$OPERF_CMD_CHECK" == "sudo: a password is required" ]; then
      print_error "cannot run oprofile under the root privileges (cannot run operf as a root), run this script as a root or add yourself to the /etc/sudoers (see help for more information)."
      exit 1
    fi

    # We need to send a SIGINT signal to the operf to stop the system-wide profiling
    KILL_CMD_CHECK=`sudo -n kill`

    if [ "$KILL_CMD_CHECK" == "sudo: a password is required" ]; then
      print_error "cannot run oprofile under the root privileges (cannot run kill as a root), run this script as a root or add yourself to the /etc/sudoers (see help for more information)."
      exit 1
    fi
  fi
}

# Program section
# ---------------

# Default values for optional parameters
RUN_TYPE=anaconda
TIME_CMD=
PROFILE=0

# Process optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--config")
      if [ -z "$2" ]; then
        print_error "missing config directory."
        exit 1
      fi
      if [ ! -d "$2" ]; then
        print_error "'"$2"' is not a directory."
        exit 1
      fi
      CONFIG_DIR=$2
      shift
      ;;
    "--run-type")
      if [ -z "$2" ]; then
        print_error "missing run type."
        exit 1
      fi
      if ! [[ "$2" =~ ^anaconda|pin|native$ ]]; then
        print_error "run type must be anaconda, pin or native."
        exit 1
      fi
      TEST_TYPE=$2
      shift
      ;;
    "--time")
      if [ -f /usr/bin/time ]; then
        TIME_CMD=/usr/bin/time
      else
        TIME_CMD=time
      fi
      ;;
    "--verbose")
      set_mode verbose
      ;;
    "--profile")
      check_oprofile
      PROFILE=1
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# An array containing information about analysers
declare -A ANALYSERS

# Import the information about analysers
for file in `find $ANALYSERS_DIR -mindepth 1 -maxdepth 1 -type f`; do
  print_verbose "loading analysers from file $file."

  source $file
done

# An array containing information about programs
declare -A PROGRAMS

# Import the information about programs
for file in `find $PROGRAMS_DIR -mindepth 1 -maxdepth 1 -type f`; do
  print_verbose "loading programs from file $file."

  source $file
done

# Setup the analyser and program to be analysed
if [ -z "$1" ]; then
  print_error "no analyser specified."
  exit 1
else
  ANALYSER_NAME=$1
fi

if [ -z "$2" ]; then
  print_error "no program specified."
  exit 1
else
  PROGRAM_NAME=$2
fi

# Get the path to the analyser
ANALYSER=${ANALYSERS[$ANALYSER_NAME]}

if [ -z "$ANALYSER" ]; then
  print_error "analyser '"$ANALYSER_NAME"' not found."
  exit 1
fi

# Get the path to the program to be analysed
PROGRAM=${PROGRAMS[$PROGRAM_NAME]}

if [ -z "$PROGRAM" ]; then
  print_error "program '"$PROGRAM_NAME"' not found."
  exit 1
fi

# Setup ANaConDA configuration
if [ -z "$CONFIG_DIR" ]; then
  CONFIG_DIR="`pwd`/conf"
fi
if [ ! -d "$CONFIG_DIR" ]; then
  print_error "directory containing ANaConDA configuration '"$CONFIG_DIR"' not found."
  exit 1
fi

# Remove old log files
rm -f *.log

# Setup and start the system-wide profiling before running the program
if [ "$PROFILE" == "1" ]; then
  # Remove old temporary files
  rm -rf operf.out

  # Start the system-wide profiling at the background
  sudo operf --system-wide --vmlinux /usr/src/linux-`uname -r`/vmlinux &> operf.out &

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

# Run the program
case "$RUN_TYPE" in
  "anaconda")
    print_verbose "executing command '$TIME_CMD $PIN_HOME/pin -t $ANACONDA_HOME/lib/intel64/anaconda --show-settings -a $ANALYSER -- $PROGRAM'."

    $TIME_CMD "$PIN_HOME/pin" -t "$ANACONDA_HOME/lib/intel64/anaconda" --show-settings -a $ANALYSER -- $PROGRAM
    ;;
  "pin")
    $TIME_CMD "$PIN_HOME/pin" -t $ANALYSER -- $PROGRAM
    ;;
  "native")
    $TIME_CMD $PROGRAM
    ;;
  *) # This should not happen, but if does better to be notified
    print_error "unknown run type '"$RUN_TYPE"'."
    exit 1
    ;;
esac

# Stop the system-wide profiling and process the results
if [ "$PROFILE" == "1" ]; then
  # Send the SIGINT signal to the operf running in the background
  eval "sudo $KILL_OPERF"

  # Process the results
  opreport -l $PIN_HOME/intel64/bin/pinbin > pin64.profile
  opreport -l $PIN_HOME/ia32/bin/pinbin > pin32.profile
  PROGRAM_ARGS=($PROGRAM)
  opreport -l "${PROGRAM_ARGS[0]}" > $PROGRAM_NAME.profile

  # Remove temporary files
  rm -rf operf.out
fi

# End of script
