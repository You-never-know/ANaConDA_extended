#!/bin/bash
#
# Description:
#   A script simplifying evaluation of performed tests.
# Author:
#   Jan Fiedor
# Version:
#   0.1
# Created:
#   05.11.2013
# Last Update:
#   06.11.2013
#

source messages.sh

# Settings section
# ----------------

# Directory in which the script was executed
SCRIPT_DIR=`pwd`

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
