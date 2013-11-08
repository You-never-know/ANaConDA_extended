#!/bin/bash
#
# Description:
#   A script simplifying evaluation of performed tests.
# Author:
#   Jan Fiedor
# Version:
#   0.7
# Created:
#   05.11.2013
# Last Update:
#   08.11.2013
#

source messages.sh

# Settings section
# ----------------

# Directory in which the script was executed
SCRIPT_DIR=`pwd`

# Directory containing information about evaluators
EVALUATORS_DIR="$SCRIPT_DIR/etc/anaconda/tools/evaluators"

# Name of a file containing basic information about a performed test
TEST_INFO_FILE="test.log"

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
#   Terminates the script.
# Parameters:
#   [STRING] A message describing the reason why the script was terminated.
# Output:
#   An error message containing the reason of the termination.
# Return:
#   Nothing
#
terminate()
{
  # Print the error (the reason of the termination)
  print_error "$1"

  # Move back to the directory in which we executed the script
  cd $SCRIPT_DIR

  exit 1
}

#
# Description:
#   Gets a unique number identifying a string.
# Parameters:
#   [STRING] A string.
#   [STRING] A name of the variable to which the number should be stored.
# Output:
#   None
# Return:
#   Nothing
#
get_id()
{
  if declare -gA &>/dev/null; then
    # Use an associative array when using Bash version 4 or newer (safer)
    if [ ${#ASSIGNED_IDS[@]} = 0 ]; then
      declare -gA ASSIGNED_IDS
    fi

    local assigned_id="${ASSIGNED_IDS[$1]}"
  else
    local assigned_id=`echo -e "$ASSIGNED_IDS" | grep -o -E "^[0-9]+\|$1$" | sed -e "s/^\([0-9]*\)|.*$/\1/"`
  fi

  if [ -z "$assigned_id" ]; then
    # If no ID assigned yet, last ID must be just before 0
    if [ -z "$LAST_ASSIGNED_ID" ]; then
      LAST_ASSIGNED_ID=-1
    fi

    # Move the the ID not assigned yet (must be unique)
    LAST_ASSIGNED_ID=$((LAST_ASSIGNED_ID+1))

    local assigned_id=$LAST_ASSIGNED_ID

    if declare -gA &>/dev/null; then
      ASSIGNED_IDS[$1]=$assigned_id
    else
      ASSIGNED_IDS="$ASSIGNED_IDS$assigned_id|$1\n"
    fi
  fi

  eval $2="'$assigned_id'"
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

  # Creating (or recreating) the list will also clear it
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
#   Evaluates a test.
# Parameters:
#   [STRING] A name of the directory contaning the results of the test.
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

  # Move to the directory contaning the test results
  cd $directory

  # Extract basic information about the test beeing evaluated
  TEST_RESULTS_DIRECTORY="$directory"
  register_evaluation_result "test-dir" TEST_RESULTS_DIRECTORY
  TESTED_PROGRAM_NAME="${directory:28}"
  register_evaluation_result "program-name" TESTED_PROGRAM_NAME
  TESTED_PROGRAM_COMMAND=`cat $TEST_INFO_FILE | grep -E -o "program=.*" | sed -e "s/^program=\(.*\)$/\1/"`
  register_evaluation_result "program-cmd" TESTED_PROGRAM_COMMAND
  PERFORMED_TEST_TYPE=`cat $TEST_INFO_FILE | grep -E -o "test-type=.*" | sed -e "s/^test-type=\(.*\)$/\1/"`
  register_evaluation_result "test-type" PERFORMED_TEST_TYPE
  USED_ANALYSER=`cat $TEST_INFO_FILE | grep -E -o "analyser=.*" | sed -e "s/^analyser=\(.*\)$/\1/"`
  register_evaluation_result "analyser" USED_ANALYSER
  NUMBER_OF_RUNS=`find . -type f -regex "^\./run[0-9]+\.out$" | wc -l`
  register_evaluation_result "runs" NUMBER_OF_RUNS

  # Extract information about the noise
  NOISE_SETTINGS=`cat ./conf/anaconda.conf | grep -E "^type|frequency|strength" | sed -e "s/type = //g" | sed -e "s/frequency = //g" | sed -e "s/strength = //g" | tail -9 | sed -e ':a;N;$!ba;s/\n/\//g'`
  register_evaluation_result "noise" NOISE_SETTINGS

  # Call the function which should be called before test evaluation
  ${BEFORE_TEST_EVALUATION[$evaluator_id]}

  for run in `find . -type f -regex "^\./run[0-9]+\.out$"`; do
    evaluate_run $run
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

# Array contaning names of variables contaning evaluation results
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
