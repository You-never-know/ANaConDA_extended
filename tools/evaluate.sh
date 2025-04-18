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
#   A script simplifying evaluation of performed tests.
# Author:
#   Jan Fiedor
# Version:
#   1.6.4
# Created:
#   05.11.2013
# Last Update:
#   27.06.2014
#

source utils.sh

# Settings section
# ----------------

# Directory containing information about evaluators
EVALUATORS_DIR="$SCRIPT_DIR/etc/anaconda/tools/evaluators"

# File containing information about a performed test
TEST_INFO_FILE="test.info"

# File containing information about performed test runs
TEST_LOG_FILE="test.log"

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
  $0 [--help] [--test <directory> | --program <name>] [--pack <archive>]
     [--process-only]

optional arguments:
  --help
    Print the script usage.
  --test <directory>
    Evaluate a single test whose results are stored in the specified directory.
  --program <name>
    Evaluate all tests of the specified program.
  --pack <archive>
    Pack the evaluated tests to an archive.
  --process-only
    Do not evaluate the tests, just process their output and print it.
"
}

#
# Description:
#   Archives a test. Adds a folder containing the test's results to an archive.
# Parameters:
#   [STRING] A path to a folder containing the test's results.
# Output:
#   None
# Return:
#   Nothing
#
archive_test()
{
  # Helper variables
  local directory=$1

  # Replace the old test's results (if present) with the current ones and delete
  # them from the file system after they were successfully added to the archive
  tar --delete -f $TAR_PATH $directory &> /dev/null
  tar --update -f $TAR_PATH --remove-files $directory
}

#
# Description:
#   Gets a unique number identifying an evaluator.
# Parameters:
#   [STRING] A name of the evaluator.
#   [STRING] A name of the variable to which the number should be stored.
# Output:
#   None
# Return:
#   Nothing
#
get_evaluator_id()
{
  get_id "$1" "$2"
}

#
# Description:
#   Registers an evaluator for a program.
# Parameters:
#   [STRING] A name of the program.
# Output:
#   None
# Return:
#   Nothing
#
register_evaluator()
{
  # Helper variables
  local evaluator_id

  # Get the number uniquely identifying the evaluator
  get_evaluator_id "$1" evaluator_id

  # Register the callback functions of the evaluator
  BEFORE_TEST_EVALUATION[$evaluator_id]=$2 # Required
  ON_TEST_RUN_VALIDATION[$evaluator_id]=$5 # Optional
  ON_TEST_RUN_EVALUATION[$evaluator_id]=$3 # Required
  AFTER_FAILED_TEST_RUN[$evaluator_id]=$6 # Optional
  AFTER_TEST_EVALUATION[$evaluator_id]=$4 # Required
}

#
# Description:
#   Clears a list.
# Parameters:
#   [STRING] A name of the list.
# Output:
#   None
# Return:
#   Nothing
#
list_clear()
{
  # Helper variables
  local list_name=$1

  # First delete the list
  unset $list_name

  # Then create it again
  declare -a $list_name
}

#
# Description:
#   Adds an item to the end of a list.
# Parameters:
#   [STRING] A name of the list.
#   [STRING] A value of the item.
# Output:
#   None
# Return:
#   Nothing
#
list_push_back()
{
  # Helper variables
  local list_name=$1
  local list_item=$2
  local list_size

  # Get the size of the list
  eval list_size="\${#$list_name[@]}"

  # Insert the item into the list
  eval $list_name[$list_size]=\"$list_item\"
}

# Description:
#   Computes an average of the items in a list.
# Parameters:
#   [STRING] A name of the list.
#   [STRING] A name of the variable to which the average should be stored. If
#            the list is empty, the result will be an empty string.
#   [STRING] A name of the variable to which the standard deviation should be
#            stored. If the list is empty, the result will be an empty string.
# Output:
#   None
# Return:
#   Nothing
#
list_average()
{
  # Helper variables
  local list_name=$1
  local list_items
  local list_size

  # Get the items in the list
  eval list_items=(\"\${$list_name[@]}\")
  # Get the size of the list
  eval list_size="\${#$list_name[@]}"

  # Operation not defined on an empty list
  if [ $list_size == 0 ]; then
    # Average value does not exist (result is an empty string)
    if [ ! -z "$2" ]; then
      eval $2="''"
    fi

    # Standard deviation cannot be computed (result is an empty string)
    if [ ! -z "$3" ]; then
      eval $3="''"
    fi

    return
  fi

  # Compute the average of the items (assume all are numbers)
  local list_items_as_string="${list_items[@]}"
  local list_items_sum=`echo "${list_items_as_string// /+}" | bc -l`
  local list_items_avg=`echo "$list_items_sum / $list_size" | bc -l`

  # Normalize the average (bc outputs numbers < 1 as .<digits>)
  if [ "${list_items_avg:0:1}" == "." ]; then
    list_items_avg="0$list_items_avg"
  fi

  # Return the average value of the items
  if [ ! -z "$2" ]; then
    eval $2="'$list_items_avg'"
  fi

  # Compute the standard deviation
  local list_items_sigma_square="0"

  # SUM_i^N (x_i - u)^2
  for list_item in "${list_items[@]}"; do
    list_items_sigma_square=`echo "$list_items_sigma_square + ($list_item - $list_items_avg)^2" | bc -l`
  done

  # SQRT(1/N * SUM_i^N (x_i - u)^2))
  local list_items_sigma=`echo "sqrt($list_items_sigma_square / $list_size)" | bc -l`

  # Normalize the standard deviation (bc outputs numbers < 1 as .<digits>)
  if [ "${list_items_sigma:0:1}" == "." ]; then
    list_items_sigma="0$list_items_sigma"
  fi

  # Return the standard deviation value of the items
  if [ ! -z "$3" ]; then
    eval $3="'$list_items_sigma'"
  fi
}

#
# Description:
#   Gets a unique number identifying an evaluation result.
# Parameters:
#   [STRING] A name used to identify the evaluation result.
#   [STRING] A name of the variable to which the number should be stored.
# Output:
#   None
# Return:
#   Nothing
#
get_evaluation_result_id()
{
  get_id "$1" "$2"
}

#
# Description:
#   Registers a variable containing a specific evaluation result.
# Parameters:
#   [STRING] A name used to identify the evaluation result.
#   [STRING] A name of a variable containing the evaluation result.
# Output:
#   None
# Return:
#   Nothing
#
register_evaluation_result()
{
  # Helper variables
  local evaluation_result_id

  # Get the number uniquely identifying the evaluation result
  get_evaluation_result_id "$1" evaluation_result_id

  # Save the name of the variable containing the evaluation result
  EVALUATION_RESULTS[$evaluation_result_id]=$2
}

#
# Description:
#   Prints values of the chosen evaluation results.
# Parameters:
#   [STRING] A list of names identifying evaluation results to print.
# Output:
#   Values of the chosen evaluation results.
# Return:
#   Nothing
#
print_evaluation_results()
{
  # Helper variables
  local evaluation_results=($1)
  local evaluation_result
  local evaluation_result_id
  local evaluation_result_value

  for evaluation_result in "${evaluation_results[@]}"; do
    # Get the number uniquely identifying the evaluation result
    get_evaluation_result_id "$evaluation_result" evaluation_result_id

    # Get the value of the evaluation result
    eval evaluation_result_value=\$${EVALUATION_RESULTS[$evaluation_result_id]}

    # Print the value of the evaluation result
    echo -n "$evaluation_result_value;"
  done

  echo
}

#
# Description:
#   Checks if a test run is valid.
# Parameters:
#   [STRING] A name of the file containing the output of the test run.
# Output:
#   None
# Return:
#   0 if the test run is valid, 1 otherwise.
#
is_valid_run()
{
  # Helper variables
  local file=$1

  # Check if the evaluator registered some validation function
  if [ ! -z "${ON_TEST_RUN_VALIDATION[$evaluator_id]}" ]; then
    # Perform the validation using the registered function
    if ! ${ON_TEST_RUN_VALIDATION[$evaluator_id]} $file; then
      return 1 # The evaluator flagged the test run as invalid
    fi
  fi

  # The test run passed the checks, consider it valid
  return 0
}

#
# Description:
#   Evaluates a test run.
# Parameters:
#   [STRING] A name of the file containing the output of the test run.
# Output:
#   None
# Return:
#   Nothing
#
evaluate_run()
{
  # Helper variables
  local file=$1

  # Call the function which should evaluate a single test run
  ${ON_TEST_RUN_EVALUATION[$evaluator_id]} $file
}

#
# Description:
#   Loads information about the currently evaluated test from its log file.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
load_test_info()
{
  # Process all information in the log file
  while read line || [[ -n "$line" ]]; do
    # The pieces of information are stored in the key=value format, get key
    local info_name=`echo $line | sed -e "s/^\(.*\)=.*$/\1/"`

    if [ ! -z "$info_name" ]; then
      # Create a name of a temporary variable which will hold the key's value
      local storage_variable="${info_name//-/_}"
      local storage_variable="$(echo "CURRENT_TEST_INFO_${storage_variable}" | tr '[:lower:]' '[:upper:]')"

      # Extract the key's value
      local info_value=`echo $line | sed -e "s/^.*=\(.*\)$/\1/"`

      # Store the value in the temporary variable
      eval $storage_variable="'$info_value'"

      # Publish the key=value pair as an evaluation result
      register_evaluation_result "$info_name" "$storage_variable"
    fi
  done < $TEST_INFO_FILE
}

#
# Description:
#   Evaluates a test.
# Parameters:
#   [STRING] A name of the directory containing the results of the test.
# Output:
#   None
# Return:
#   Nothing
#
evaluate_test()
{
  # Helper variables
  local directory=$1
  local program=$(echo $(basename $directory) | sed -e "s/^[0-9T.]*-//")
  local evaluator_id

  # Get the number uniquely identifying the evaluator we should use
  get_evaluator_id "$program" evaluator_id

  # Move to the directory containing the test results
  cd $directory

  # Save the identification of the test being evaluated
  TEST_RESULTS_DIRECTORY="$directory"
  register_evaluation_result "test-results-dir" TEST_RESULTS_DIRECTORY

  # Extract information about the test being evaluated
  load_test_info

  # Extract information about the test runs performed
  PERFORMED_TEST_RUNS=`cat $TEST_LOG_FILE | wc -l`
  register_evaluation_result "performed-test-runs" PERFORMED_TEST_RUNS
  SUCCEEDED_TEST_RUNS=`cat $TEST_LOG_FILE | grep "succeeded" | wc -l`
  register_evaluation_result "succeeded-test-runs" SUCCEEDED_TEST_RUNS
  TIMEOUTED_TEST_RUNS=`cat $TEST_LOG_FILE | grep "timeouted" | wc -l`
  register_evaluation_result "timeouted-test-runs" TIMEOUTED_TEST_RUNS
  FAILED_TEST_RUNS=`cat $TEST_LOG_FILE | grep "failed" | wc -l`
  register_evaluation_result "failed-test-runs" FAILED_TEST_RUNS
  INVALID_TEST_RUNS=0 # Computed dynamically during the evaluation
  register_evaluation_result "invalid-test-runs" INVALID_TEST_RUNS

  # Extract information about the noise
  NOISE_SETTINGS=`test -f ./conf/anaconda.conf && cat ./conf/anaconda.conf | grep -E "^type|frequency|strength" | sed -e "s/type = //g" | sed -e "s/frequency = //g" | sed -e "s/strength = //g" | tail -9 | sed -e ':a;N;$!ba;s/\n/\//g'`
  register_evaluation_result "noise" NOISE_SETTINGS

  # Prepare information specific to each test run
  TEST_RUN_ID=0 # Computed dynamically during the evaluation
  register_evaluation_result "test-run-id" TEST_RUN_ID

  # Get the number of test runs executed (not all might be evaluated in the end)
  local executed_runs=`find . -type f -regex "^\./run[0-9]+\.out$" | wc -l`

  # Call the function which should be called before test evaluation
  ${BEFORE_TEST_EVALUATION[$evaluator_id]} $executed_runs

  # A counter holding the number of test runs evaluated (that passed all checks)
  local evaluated_runs=0

  # Evaluate the test runs
  for ((executed_run = 0; executed_run < $executed_runs; executed_run++)); do
    # Set the ID of the currently evaluated test run
    TEST_RUN_ID=$executed_run

    # Determine how the test run ended (succeeded, timeouted or failed)
    RUN_RESULT=`cat $TEST_LOG_FILE | grep -o -E "^run $executed_run: [a-zA-Z]+" | sed -e "s/^run [0-9]*: \([a-zA-Z]*\)/\1/"`
    register_evaluation_result "test-run-result" RUN_RESULT

    # Skip failed and timeouted runs, they might contain invalid results
    if [ "$RUN_RESULT" != "succeeded" ]; then
      # Test run failed or timeouted, cannot be evaluated
      ${AFTER_FAILED_TEST_RUN[$evaluator_id]}

      continue
    fi

    # Get a path to the file containing the output of a test run
    TEST_RUN_OUTPUT_FILE=`printf "./run%.10d.out" $executed_run`

    # Validate the test run before evaluating it
    if ! is_valid_run $TEST_RUN_OUTPUT_FILE; then
      INVALID_TEST_RUNS=$((INVALID_TEST_RUNS+1))
      RUN_RESULT="invalid"

      # Test run is invalid, cannot be evaluated
      ${AFTER_FAILED_TEST_RUN[$evaluator_id]}

      continue # Skip test runs flagged as invalid
    fi

    # Evaluate a single test run
    evaluate_run $TEST_RUN_OUTPUT_FILE

    # The current test run was evaluated
    evaluated_runs=$((evaluated_runs+1))
  done

  # Call the function which should be called after test evaluation
  ${AFTER_TEST_EVALUATION[$evaluator_id]} $evaluated_runs

  # Move back to the directory in which we executed the script
  cd $SCRIPT_DIR
}

# Program section
# ---------------

# Default values for optional parameters
EVALUATION_MODE=evaluate
EVALUATION_TYPE=all
ARCHIVE_PATH=

# Process the optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--test")
      if [ "$EVALUATION_TYPE" == "program" ]; then
        terminate "--test cannot be used with --program."
      fi
      if [ -z "$2" ]; then
        terminate "missing directory contaning test results."
      fi
      EVALUATION_TYPE=test
      TEST_DIRECTORY=$2
      shift
      ;;
    "--program")
      if [ "$EVALUATION_TYPE" == "test" ]; then
        terminate "--program cannot be used with --test."
      fi
      if [ -z "$2" ]; then
        terminate "missing program name."
      fi
      EVALUATION_TYPE=program
      PROGRAM_NAME=$2
      shift
      ;;
    "--pack")
      if [ -z "$2" ]; then
        terminate "missing path to archive."
      fi
      ARCHIVE_PATH=$2
      shift
      ;;
    "--process-only")
      EVALUATION_MODE=process
      shift
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Arrays containing callback functions of registered evaluators
declare -a BEFORE_TEST_EVALUATION
declare -a ON_TEST_RUN_VALIDATION
declare -a ON_TEST_RUN_EVALUATION
declare -a AFTER_FAILED_TEST_RUN
declare -a AFTER_TEST_EVALUATION

# Import the information about evaluators
for file in `find $EVALUATORS_DIR -mindepth 1 -maxdepth 1 -type f`; do
  source $file
done

# Array containing names of variables containing evaluation results
declare -a EVALUATION_RESULTS

# Check if the given archive already exists
if [ -f "$ARCHIVE_PATH" ]; then
  # Unpack the archive first (if it is compressed)
  case "$ARCHIVE_PATH" in
    *.gz|*.tgz|*.taz) # Gzip compression
      TAR_PATH=`gunzip -f -v $ARCHIVE_PATH 2>&1 | sed -e "s/^$ARCHIVE_PATH:.*-- replaced with \(.*\)$/\1/"`
      ;;
    *.tar) # No compression
      TAR_PATH=$ARCHIVE_PATH
      ;;
  esac
else
  TAR_PATH="$ARCHIVE_PATH.tar"
fi

# Evaluate the performed test(s)
if [ "$EVALUATION_TYPE" == "test" ]; then
  # Evaluate a single test
  if [ ! -d "$TEST_DIRECTORY" ]; then
    terminate "directory $TEST_DIRECTORY not found."
  else
    evaluate_test $TEST_DIRECTORY
  fi
else
  # Evaluate a set of tests
  for test_dir in `find . -mindepth 1 -maxdepth 1 -type d`; do
    # Skip directories not containing test results
    if [ ! -f "$test_dir/test.info" ]; then
      continue
    fi

    if [ "$EVALUATION_TYPE" == "program" ]; then
      # Evaluate only tests of the specified program, filter out the others
      prog_name=$(echo $(basename $test_dir) | sed -e "s/^[0-9T.]*-//")

      if [ "$prog_name" != "$PROGRAM_NAME" ]; then
        continue
      fi
    fi

    evaluate_test $test_dir

    # Archive the test's results if requested
    if [ ! -z "$ARCHIVE_PATH" ]; then
      archive_test $test_dir
    fi
  done
fi

# Check if an archive is present
if [ -f "$TAR_PATH" ]; then
  # Compress the archive
  case "$ARCHIVE_PATH" in
    *.gz|*.tgz|*.taz) # Gzip compression
      TAR_PATH=`gzip -f -v $TAR_PATH 2>&1 | sed -e "s/^$TAR_PATH:.*-- replaced with \(.*\)$/\1/"`
      mv -f $TAR_PATH $ARCHIVE_PATH &> /dev/null
      ;;
    *) # No compression
      mv -f $TAR_PATH $ARCHIVE_PATH &> /dev/null
      ;;
  esac
fi

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
