#!/bin/bash
#
# Description:
#   A script simplifying testing programs with ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   0.9.1
# Created:
#   27.03.2013
# Last Update:
#   14.11.2013
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
     <analyser> <program>

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
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Prepare the analyser
setup_analyser $1

# Prepare the program
setup_program $2

# Propare the environment
setup_environment

# Prepare a directory in which the test runs will be performed
TIMESTAMP=`date "+%Y%m%dT%H%M%S.%N"`
TEST_DIRECTORY="`pwd`/$TIMESTAMP-$PROGRAM_NAME"
mkdir "$TEST_DIRECTORY"
cd $TEST_DIRECTORY

# Prepare ANaConDA settings
setup_config

# Determine the number of test runs
if [ "$TEST_TIME" -gt "0" ]; then
  RUNS=$(($TEST_TIME * 1000))

  # Start a watchdog interrupting the test after the time runs out
  (test_time_watchdog $TEST_TIME) &

  # Save the PID of the watchdog to kill it if neccessary
  TEST_TIME_WATCHDOG_PID=$!
elif [ "$TEST_RUNS" -gt "0" ]; then
  RUNS=$TEST_RUNS
else
  RUNS=1
fi

print_section "Performing test $PROGRAM"

# Save information about the test performed
save_test_info

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

  # Check if the test run finished with or without an error
  if [ "$?" -eq "0" ]; then
    echo "run $RUN: succeeded" >> $FINISHED_RUNS_LOG_FILE
  else
    echo "run $RUN: failed (error code $?)" >> $FINISHED_RUNS_LOG_FILE
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

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
