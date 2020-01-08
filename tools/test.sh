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
#   A script simplifying testing programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   1.1.3
# Created:
#   27.03.2013
# Last Update:
#   20.11.2013
#

source executions.sh

# Settings section
# ----------------

# File containing information about the test performed
TEST_INFO_FILE="test.info"

# File containing information about the performed test runs
TEST_LOG_FILE="test.log"

# File containing information about the timeouted test runs
TIMEOUTED_RUNS_LOG_FILE="test.timeouted"

# File containing information about the finished test runs
FINISHED_RUNS_LOG_FILE="test.finished"

# File containing information about the watchdogs
WATCHDOGS_LOG_FILE="watchdogs.log"

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
  $0 [--help] [--test-type { anaconda | pin | native }] [--config <dir>]
     [--time <seconds> | --runs <number> ] [--timeout <seconds>]
     [--threads <number>] <analyser> <program>

required arguments:
  <analyser>  A name of the analyser to be used.
  <program>   A name of the program to be analysed.

optional arguments:
  --help
    Print the script usage.
  --test-type { anaconda | pin | native }
    Execute the program in ANaConDA, PIN or no framework (native run). Default
    is to run the program in ANaConDA.
  --config <dir>
    A path to a directory containing ANaConDA settings.
  --time <seconds>
    A time limit for the whole test. When the time runs out, the test will be
    interrupted.
  --runs <number>
    A number of test runs to perform.
  --timeout <seconds>
    A time limit for each test run. When the time runs out, the current test
    run is interrupted (and next test run is started).
  --threads
    A number of threads the analysed program should utilize. If not specified,
    the number of cores available will be used as the number for threads. Note
    that this number is just a recommendation and the target program might use
    a different number or no number at all (if the number of threads cannot be
    set at all as the program just always use how many threads are necessary).
    When registering a program, one may use the THREADS variable to access the
    recommended number of threads the program should utilize.
"
}

#
# Description:
#   Stops the script.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
on_interrupt()
{
  # Stop the currently performed test run (kill the tested program)
  killall -9 -u `whoami` $PROGRAM_NAME 2>&1 &> /dev/null

  # Stop the watchdog interrupting the test run after the time runs out
  stop_test_run_timeout_watchdog

  # Validate the log files
  validate_logs

  # Move back to the directory in which we executed the script
  cd $SCRIPT_DIR

  # Stop the test script
  exit 0
}

#
# Description:
#   Notifies the script when the time given to it runs out.
# Parameters:
#   [NUMBER] A number of seconds the script has to perform the test runs.
# Output:
#   None
# Return:
#   Nothing
#
test_time_watchdog()
{
  # Helper variables
  local time=$1

  # Suspend the watchdog until the time runs out
  sleep $time

  # Notify the script that the time ran out
  kill -SIGINT $$ 2>&1 &> /dev/null

  print_section "Time for the test expired..."
}

#
# Description:
#   Kills the tested program if the time given to it runs out.
# Parameters:
#   [NUMBER] A number identifying the test run.
#   [NUMBER] A number of seconds the tested program has to finish its execution.
#   [STRING] A name of the tested program's executable (used to kill it).
# Output:
#   None
# Return:
#   Nothing
#
test_run_timeout_watchdog()
{
  # Helper variables
  local run=$1
  local timeout=$2
  local program=$3

  # Suspend the watchdog until the time runs out
  sleep $timeout

  # As we do not have the PID of the tested program (we do not start it, PIN
  # does it), we have to kill the program by its name instead of its PID
  killall -9 -u `whoami` $program 2>&1 &> /dev/null

  # Log the fact that we killed the program because the time runs out
  echo "run $run: timeouted" >> $TIMEOUTED_RUNS_LOG_FILE
}

#
# Description:
#   Starts a watchdog which kills the tested program if the time given to it
#   runs out.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
start_test_run_timeout_watchdog()
{
  if [ "$TEST_RUN_TIMEOUT" -gt "0" ]; then
    (test_run_timeout_watchdog $RUN $TEST_RUN_TIMEOUT $PROGRAM_NAME) &

    TEST_RUN_TIMEOUT_WATCHDOG_PID=$!

    echo "run $RUN: started test run timeout watchdog (pid=$TEST_RUN_TIMEOUT_WATCHDOG_PID)" >> $WATCHDOGS_LOG_FILE
  fi
}

#
# Description:
#   Stops a watchdog which kills the tested program if the time given to it
#   runs out.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
stop_test_run_timeout_watchdog()
{
  if [ "$TEST_RUN_TIMEOUT" -gt "0" ]; then
    if kill -9 $TEST_RUN_TIMEOUT_WATCHDOG_PID 2>&1 &> /dev/null; then
      echo "run $RUN: stopped test run timeout watchdog (pid=$TEST_RUN_TIMEOUT_WATCHDOG_PID)" >> $WATCHDOGS_LOG_FILE
    fi
  fi
}

#
# Description:
#   Saves information about the test performed to a log file.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
save_test_info()
{
  echo "\
test-type=$TEST_TYPE
test-time=$TEST_TIME
test-runs=$TEST_RUNS
test-run-timeout=$TEST_RUN_TIMEOUT
analyser=$ANALYSER
analyser-name=$ANALYSER_NAME
analyser-path=$ANALYSER_PATH
analyser-command=$ANALYSER_COMMAND
program=$PROGRAM
program-name=$PROGRAM_NAME
program-path=$PROGRAM_PATH
program-command=$PROGRAM_COMMAND
config-dir=$CONFIG_DIR
" > $TEST_INFO_FILE
}

#
# Description:
#   Setups ANaConDA settings.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
setup_config()
{
  # Use the default settings directory if no directory specified
  if [ -z "$CONFIG_DIR" ]; then
    CONFIG_DIR="$SCRIPT_DIR/conf"
  fi

  # Check if the settings directory exist
  if [ ! -d "$CONFIG_DIR" ]; then
    terminate "directory $CONFIG_DIR containing ANaConDA settings not found."
  fi

  # Move the settings to the directory where the test runs will be executed
  mkdir "$TEST_DIRECTORY/conf"
  cp -R "$CONFIG_DIR/"* "$TEST_DIRECTORY/conf"

  # Setup the coverage (workaround for referencing previous coverage results)
  mkdir "$TEST_DIRECTORY/coverage"
  echo -n > "$TEST_DIRECTORY/coverage/19700101T000000.000001-$PROGRAM_NAME.svars"
  echo -n > "$TEST_DIRECTORY/coverage/19700101T000000.000001-$PROGRAM_NAME.preds"
}

#
# Description:
#   Initializes logs.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
init_logs()
{
  echo -n > $TEST_LOG_FILE
  echo -n > $TIMEOUTED_RUNS_LOG_FILE
  echo -n > $FINISHED_RUNS_LOG_FILE
  echo -n > $WATCHDOGS_LOG_FILE
}

#
# Description:
#   Validates logs.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
validate_logs()
{
  # Helper variables
  local executed_runs=`find . -type f -regex "^\./run[0-9]+\.out$" | wc -l`

  # Process the logs containing info about the finished and timeouted test runs
  for ((executed_run = 0; executed_run < $executed_runs; executed_run++)); do
    # Get the result of a test run
    local run_result=`cat $FINISHED_RUNS_LOG_FILE | grep -o -E "^run $executed_run: [a-zA-Z]+" | sed -e "s/^run [0-9]*: \([a-zA-Z]*\)/\1/"`

    if [ "$run_result" == "succeeded" ]; then
      # Test run finished without errors
      echo "run $executed_run: succeeded" >> $TEST_LOG_FILE
    elif [ "$run_result" == "failed" ]; then
      # Test run finished with an error
      local timeout_result=`cat $TIMEOUTED_RUNS_LOG_FILE | grep -o -E "^run $executed_run: [a-zA-Z]+" | sed -e "s/^run [0-9]*: \([a-zA-Z]*\)/\1/"`

      if [ "$timeout_result" == "timeouted" ]; then
        # The error was caused by us killing the program because of a timeout
        echo "run $executed_run: timeouted" >> $TEST_LOG_FILE
      else
        # The error was caused by the program itself
        echo "run $executed_run: failed" >> $TEST_LOG_FILE
      fi
    fi
  done
}

# Program section
# ---------------

# Default values for optional parameters
TEST_TYPE=anaconda
TEST_TIME=0
TEST_RUNS=0
TEST_RUN_TIMEOUT=0

# Initialize environment first, optional parameters might override the values
env_init

# Process optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--test-type")
      if [ -z "$2" ]; then
        terminate "missing test type."
      fi
      if ! [[ "$2" =~ ^anaconda|pin|native$ ]]; then
        terminate "test type must be anaconda, pin or native."
      fi
      TEST_TYPE=$2
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
      if [ "$TEST_RUNS" -gt "0" ]; then
        terminate "--time cannot be used together with --runs."
      fi
      if [ -z "$2" ]; then
        terminate "missing time value."
      fi
      if ! [[ "$2" =~ ^[0-9]+$ ]]; then
        terminate "time value must be a number."
      fi
      TEST_TIME=$2
      shift
      ;;
    "--runs")
      if [ "$TEST_TIME" -gt "0" ]; then
        terminate "--runs cannot be used together with --time."
      fi
      if [ -z "$2" ]; then
        terminate "missing number of runs."
      fi
      if ! [[ "$2" =~ ^[0-9]+$ ]]; then
        terminate "number of runs must be a number."
      fi
      TEST_RUNS=$2
      shift
      ;;
    "--timeout")
      if [ -z "$2" ]; then
        terminate "missing timeout value."
      fi
      if ! [[ "$2" =~ ^[0-9]+$ ]]; then
        terminate "timeout value must be a number."
      fi
      TEST_RUN_TIMEOUT=$2
      shift
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
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Determine the number of threads
if [ -z "$THREADS" ]; then
  # Try to utilize all the processors
  THREADS=$NUMBER_OF_CORES
fi

# Prepare the analyser
if [ "$TEST_TYPE" != "native" ]; then
  setup_analyser $1
fi

# Prepare the program
setup_program $2

# Prepare the environment
setup_environment

# Prepare a directory in which the test runs will be performed
TIMESTAMP=`date "+%Y%m%dT%H%M%S.%N"`
TEST_DIRECTORY="`pwd`/$TIMESTAMP-$PROGRAM_NAME"
mkdir "$TEST_DIRECTORY"
cd $TEST_DIRECTORY

# Prepare ANaConDA settings
if [ "$TEST_TYPE" == "anaconda" ]; then
  setup_config
fi

# Determine the number of test runs
if [ "$TEST_TIME" -gt "0" ]; then
  RUNS=$(($TEST_TIME * 1000))

  # Start a watchdog interrupting the test after the time runs out
  (test_time_watchdog $TEST_TIME) &

  # Save the PID of the watchdog to kill it if necessary
  TEST_TIME_WATCHDOG_PID=$!
elif [ "$TEST_RUNS" -gt "0" ]; then
  RUNS=$TEST_RUNS
else
  RUNS=1
fi

print_section "Preparing test script..."

print_subsection "checking test environment"

print_info "     checking killall command... " -n

if killall --version 2>&1 &>/dev/null; then
  print_info "found"
else
  print_info "not found"
  terminate "killall command not found."
fi

print_section "Performing test..."

# Save information about the test performed
save_test_info

# Initialize the log files
init_logs

# Setup a signal handler
trap on_interrupt SIGINT

# Run the test (execute the test runs)
for ((RUN = 0; RUN < $RUNS; RUN++)); do
  print_subsection "executing run $RUN..."

  # File containing the output of the test run
  OUTPUT_FILE=`printf "./run%.10d.out" $RUN`

  # Start a watchdog interrupting the test run after the time runs out
  start_test_run_timeout_watchdog

  # Execute the test run
  case "$TEST_TYPE" in
    "anaconda")
      (/usr/bin/time -a -o $OUTPUT_FILE "$PIN_HOME/pin.sh" -t "$ANACONDA_HOME/lib/intel64/anaconda" --show-settings -a $ANALYSER_COMMAND -- $PROGRAM_COMMAND 2>&1 &> $OUTPUT_FILE) &
      ;;
    "pin")
      (/usr/bin/time -a -o $OUTPUT_FILE "$PIN_HOME/pin.sh" -t $ANALYSER_COMMAND -- $PROGRAM_COMMAND 2>&1 &> $OUTPUT_FILE) &
      ;;
    "native")
      (/usr/bin/time -a -o $OUTPUT_FILE $PROGRAM_COMMAND 2>&1 &> $OUTPUT_FILE) &
      ;;
    *) # This should not happen, but if does better to be notified
      terminate "unknown test type $TEST_TYPE."
      ;;
  esac

  # Wait for the test run to finish
  wait $!

  # The time command should return the exit status of the invoked program (PIN
  # or the tested program) and PIN should return the exit status of the tested
  # program, that means that the $? variable should contain the exit status of
  # the tested program whether we executed it natively, in PIN, or in ANaConDA
  PROGRAM_EXIT_STATUS=$?

  # Check if the test run finished with or without an error
  if [ "$PROGRAM_EXIT_STATUS" -eq "0" ]; then
    echo "run $RUN: succeeded" >> $FINISHED_RUNS_LOG_FILE
  else
    echo "run $RUN: failed (error code $PROGRAM_EXIT_STATUS)" >> $FINISHED_RUNS_LOG_FILE
  fi
  
  # Stop the watchdog interrupting the test run after the time runs out
  stop_test_run_timeout_watchdog

  print_subsection "run $RUN finished..."
done

# Stop the watchdog interrupting the test after the time runs out
if [ "$TEST_TIME" -gt "0" ]; then
  kill -9 $TEST_TIME_WATCHDOG_PID 2>&1 &> /dev/null
fi

print_section "All test runs finished..."

# Validate the log files
validate_logs

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
