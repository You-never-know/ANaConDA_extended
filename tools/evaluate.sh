#!/bin/bash
#
# Description:
#   A script simplifying evaluation of performed tests.
# Author:
#   Jan Fiedor
# Version:
#   1.2
# Created:
#   05.11.2013
# Last Update:
#   18.11.2013
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
  $0 [--help] {--test <directory> | --program <name>}

optional arguments:
  --help
    Print the script usage.
  --test <directory>
    Evaluate a single test whose results are stored in the specified directory.
  --program <name>
    Evaluate all tests of the specified program.
"
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
  BEFORE_TEST_EVALUATION[$evaluator_id]=$2
  ON_TEST_RUN_EVALUATION[$evaluator_id]=$3
  AFTER_TEST_EVALUATION[$evaluator_id]=$4
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
#   [STRING] A name of the variable to which the average should be stored.
#   [STRING] A name of the variable to which the standard deviation should be
#            stored.
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

  # Empty list, nothing to do
  if [ $list_size == 0 ]; then
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

  # Extract information about the noise
  NOISE_SETTINGS=`cat ./conf/anaconda.conf | grep -E "^type|frequency|strength" | sed -e "s/type = //g" | sed -e "s/frequency = //g" | sed -e "s/strength = //g" | tail -9 | sed -e ':a;N;$!ba;s/\n/\//g'`
  register_evaluation_result "noise" NOISE_SETTINGS

  # Call the function which should be called before test evaluation
  ${BEFORE_TEST_EVALUATION[$evaluator_id]}

  # Helper variables
  local executed_runs=`find . -type f -regex "^\./run[0-9]+\.out$" | wc -l`

  # Evaluate the test runs
  for ((executed_run = 0; executed_run < $executed_runs; executed_run++)); do
    # Determine how the test run ended (succeeded, timeouted or failed)
    local run_result=`cat $TEST_LOG_FILE | grep -o -E "^run $executed_run: [a-zA-Z]+" | sed -e "s/^run [0-9]*: \([a-zA-Z]*\)/\1/"`

    # Skip failed and timeouted runs, they might contain invalid results
    if [ "$run_result" != "succeeded" ]; then
      continue
    fi

    # Evaluate a single test run
    evaluate_run `printf "./run%.10d.out" $executed_run`
  done

  # Call the function which should be called after test evaluation
  ${AFTER_TEST_EVALUATION[$evaluator_id]}

  # Move back to the directory in which we executed the script
  cd $SCRIPT_DIR
}

# Program section
# ---------------

# Default values for optional parameters
EVALUATION_TYPE=all

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
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Arrays containing callback functions of registered evaluators
declare -a BEFORE_TEST_EVALUATION
declare -a ON_TEST_RUN_EVALUATION
declare -a AFTER_TEST_EVALUATION

# Import the information about evaluators
for file in `find $EVALUATORS_DIR -mindepth 1 -maxdepth 1 -type f`; do
  source $file
done

# Array containing names of variables containing evaluation results
declare -a EVALUATION_RESULTS

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
  done
fi

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
