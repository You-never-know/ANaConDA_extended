#!/bin/bash
#
# Description:
#   Utility functions shared among the scripts.
# Author:
#   Jan Fiedor
# Version:
#   1.0
# Created:
#   09.11.2013
# Last Update:
#   09.11.2013
#

# Functions section
# -----------------

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

# End of script
