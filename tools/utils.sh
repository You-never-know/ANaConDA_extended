#!/bin/bash
#
# Description:
#   Utility functions shared among the scripts.
# Author:
#   Jan Fiedor
# Version:
#   1.1.1
# Created:
#   09.11.2013
# Last Update:
#   09.11.2013
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
#   Initializes environment.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
init_env()
{
  # Load the environment variables
  source ~/.anaconda/environment
}

#
# Description:
#   Escapes characters treated as special characters by the sed command.
# Parameters:
#   [STRING] A string.
# Output:
#   A string with escaped special characters.
# Return:
#   Nothing
#
sed_escape_special_chars()
{
  echo "$1" | sed 's!\([]\*\$\/&[]\)!\\\1!g'
}

#
# Description:
#   Updates an environment variable.
# Parameters:
#   [STRING] A name of the environment variable.
#   [STRING] A value of the environment variable.
# Output:
#   None
# Return:
#   Nothing
#
update_env_var()
{
  # Helper variables
  local environment_file=~/.anaconda/environment

  if [ ! -f "$environment_file" ]; then
    mkdir -p `dirname $environment_file`
    echo -n > $environment_file
  fi

  # Escape the name and value so we can use them with the sed command
  local sed_escaped_name="$(sed_escape_special_chars "$1")"
  local sed_escaped_value="$(sed_escape_special_chars "$2")"

  # Update the variable in the environment file first
  cat $environment_file | grep -E "^$1=" >/dev/null && sed -i -e "s/^$sed_escaped_name=.*$/$sed_escaped_name=$sed_escaped_value/" $environment_file || echo "$1=$2" >> $environment_file

  # Update the variable in the current environment
  export $1=$2
}

#
# Description:
#   Reconfigures the environment to use a specific GCC compiler.
# Parameters:
#   [STRING] A path to a directory where a GCC compiler was installed.
# Output:
#   None
# Return:
#   None
#
switch_gcc()
{
  # Helper variables
  local gcc_home=$1
  local IFS=':'

  # Get the diretories which the system searches for the GCC compiler
  local path_items=($PATH)

  # Prefer the specified GCC compiler if it is not already preferred
  if [ "${path_items[0]}" != "$gcc_home/bin" ]; then  
    if [ -z "$PATH" ]; then
      export PATH="$gcc_home/bin"
    else
      export PATH="$gcc_home/bin:$PATH"
    fi
  fi

  # Get the directories which the system searches for the libraries
  local ld_library_path_items=($LD_LIBRARY_PATH)

  # Get the target architecture of the GGC compiler specified
  local IFS='-'
  local arch=(`$gcc_home/bin/g++ -dumpmachine`)

  # Libraries for the 64-bit applications are in the lib64 folder
  if [ "$arch" == "x86_64" ]; then
    local lib_dir="lib64"
  else
    local lib_dir="lib"
  fi

  # Prefer the libraries which belong to the specified GCC compiler
  if [ "${ld_library_path_items[0]}" != "$gcc_home/$lib_dir" ]; then
    if [ -z "$LD_LIBRARY_PATH" ]; then
      export LD_LIBRARY_PATH="$gcc_home/$lib_dir"
    else
      export LD_LIBRARY_PATH="$gcc_home/$lib_dir:$LD_LIBRARY_PATH"
    fi
  fi  
}

# End of script
