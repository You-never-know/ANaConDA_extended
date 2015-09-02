#!/bin/bash
#
# Description:
#   Utility functions shared among the scripts.
# Author:
#   Jan Fiedor
# Version:
#   1.7
# Created:
#   09.11.2013
# Last Update:
#   02.09.2015
#

source messages.sh

# Settings section
# ----------------

# Directory in which the script was executed
SCRIPT_DIR=`pwd`

# File containing values of environment variables
ENVIRONMENT_FILE="$HOME/.anaconda/environment"

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
#   Checks if a list contains a specific item.
# Parameters:
#   [STRING] A list.
#   [STRING] An item which the list should contain.
#   [STRING] A separator used to separate items in the list. If no separator is
#            specified, white spaces will be treated as separators.
# Output:
#   None
# Return:
#   0 if the list contains the specified item, 1 otherwise.
#
list_contains()
{
  # Helper variables
  local list=$1
  local searched_item=$2
  local item=

  # If a separator is specified, override the global one
  if [ ! -z "$3" ]; then
    local IFS="$3"
  fi

  # Search for the item specified
  for item in $list; do
    if [ "$item" == "$searched_item" ]; then
      return 0 # Item found
    fi
  done

  return 1 # Item not found
}

#
# Description:
#   Removes duplicate items from a list.
# Parameters:
#   [STRING] A list.
#   [STRING] A separator used to separate items in the list. If no separator is
#            specified, white spaces will be treated as separators.
# Output:
#   A new list without any duplicate items.
# Return:
#   Nothing
#
list_remove_duplicates()
{
  # Helper variables
  local list=$1
  local separator=$2
  local item=

  # If a separator is specified, override the global one
  if [ ! -z "$separator" ]; then
    local IFS="$separator"
  fi

  # Create a new list without any duplicates
  for item in $list; do
    if ! list_contains "$result" "$item" "$separator"; then
      local result="$result$separator$item"
    fi
  done

  # Write the list to a standard output
  echo -n "${result:1}"
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
#   Corrects paths when using the scripts in a mixed environment.
# Parameters:
#   [LIST] A list of names of variables containing the path to be corrected.
# Output:
#   None
# Return:
#   Nothing
#
correct_paths()
{
  for path in "$@"; do
    # Transform the Cygwin paths to a Windows ones
    local corrected_path=`echo "${!path}" | sed -e "s/^\/cygdrive\/\([a-zA-Z]\)\(.*\)$/\1:\2/"`
    # Update the varible with the corrected path
    eval "$path=\"$corrected_path\""
  done
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
env_init()
{
  # Helper variables
  local environment_file="$ENVIRONMENT_FILE"

  if [ ! -f "$environment_file" ]; then
    mkdir -p `dirname $environment_file`
    echo -n > $environment_file
  fi

  # Load the environment variables
  source "$environment_file"

  # Export the environment variables
  while read line || [[ -n "$line" ]]; do
    local env_var=`echo $line | sed -e "s/^\(.*\)=.*$/\1/"`

    if [ ! -z "$env_var" ]; then
      export $env_var
    fi
  done < $environment_file
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
env_update_var()
{
  # Helper variables
  local environment_file="$ENVIRONMENT_FILE"

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
#   [STRING] A path to a directory where the GCC compiler was installed.
# Output:
#   None
# Return:
#   None
#
switch_gcc()
{
  # Helper variables
  local gcc_home=$1
  local lib_search_path=

  # Prefer the specified GCC compiler (add it to the search paths)
  if [ -d "$gcc_home/bin" ]; then
    export PATH=$(list_remove_duplicates "$gcc_home/bin:$PATH" ":")
  fi

  # Prefer the libraries which belong to the specified GCC compiler
  for lib_search_path in `ld --verbose | grep SEARCH | sed -e "s/SEARCH_DIR(\"\([^\"]*\)\");[ ]*/\1\n/g"`; do
    if [ -d "$lib_search_path" ]; then
      export LD_LIBRARY_PATH=$(list_remove_duplicates "$lib_search_path:$LD_LIBRARY_PATH" ":")
    fi 
  done
}

#
# Description:
#   Processes all configuration files in a directory given as a relative path.
#   The following directories are used as a base directories for the relative
#   path specified and are listed in the order in which they are searched:
#   1) The $SOURCE_DIR/tools/conf directory (default configuration)
#   2) The /etc/anaconda directory (system-wide configuration)
#   3) The ~/.anaconda directory (user-specific configuration)
#   4) The current directory (local configuration)
#   The order ensures that the lowest priority configuration (default config)
#   is processed first and then it is overwritten by the configurations with
#   higher priorities (system-wide -> user-specific -> local).
# Parameters:
#   [STRING] A relative path to a directory.
#   [FUNCTION] A name of a function which will be executed for each file found.
#              This function should take a single parameter holding the path to
#              the configuration file that should be processed.
# Output:
#   None
# Return:
#   None
#
process_config_dir()
{
  # Helper variable
  local path=$1
  local callback=$2

  # A list of directories which will be searched for the relative path
  local base_dirs=("$SOURCE_DIR/tools/conf" "/etc/anaconda" "$HOME/.anaconda" "$PWD")

  # Search all the directories for the relative path and the contained files
  for base_dir in ${base_dirs[@]}; do
    if [ -d "$base_dir/$path" ]; then
      for file in `find "$base_dir/$path" -mindepth 1 -maxdepth 1 -type f`; do
        # Call the function which will process the found configuration file
        $callback $file
      done
    fi
  done
}

# End of script
