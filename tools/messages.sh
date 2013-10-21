#!/bin/bash
#
# Description:
#   Functions for formating and printing arbitrary messages.
# Author:
#   Jan Fiedor
# Version:
#   2.2
# Created:
#   26.03.2010
# Last Update:
#   21.10.2013
#

# Settings section
# ----------------

#
#   Mode      ID  Description
#   ----      --  -----------
#   silent    0   Do not print any messages.
#   quiet     1   Print only error and warning messages.
#   mormal    2   Print all messages except the verbose and debug messages.
#   verbose   3   Print all messages except the debug messages.
#   debug     4   Print all messages.
#
MODE=2

# Functions section
# -----------------

#
# Description:
#   Sets a mode determining which types of messages will be printed.
# Parameters:
#   [STRING] A name of the mode. See the mode table above.
# Return:
#   Nothing
#
set_mode()
{
  case "$1" in
    "silent")
      MODE=0
      ;;
    "quiet")
      MODE=1
      ;;
    "normal")
      MODE=2
      ;;
    "verbose")
      MODE=3
      ;;
    "debug")
      MODE=4
      ;;
    *)
      print_error "unknown mode '"$1"'."
      exit 1
      ;;
  esac
}

#
# Description:
#   Prints an error message preceded by a red 'error: ' text to a standard error
#   stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_error()
{
  if [[ $MODE -ge 1 ]]; then
    if [ -c /dev/stderr ]; then
      echo -e "\e[1;31merror: \e[0m$1" 1>&2
    else
      echo -e "error: $1" 1>&2
    fi
  fi
}

#
# Description:
#   Prints a warning message preceded by a yellow 'warning: ' text to a standard
#   output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_warning()
{
  if [[ $MODE -ge 1 ]]; then
    if [ -c /dev/stdout ]; then
      echo -e "\e[1;33mwarning: \e[0m$1"
    else
      echo -e "warning: $1"
    fi
  fi
}

#
# Description:
#   Prints an information message to a standard output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_info()
{
  if [[ $MODE -ge 2 ]]; then
    echo -e "$1"
  fi
}

#
# Description:
#   Prints a white section message preceded by a blue ':: ' text to a standard
#   output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_section()
{
  if [[ $MODE -ge 2 ]]; then
    if [ -c /dev/stdout ]; then
      echo -e "\e[1;34m:: \e[1;37m$1\e[0m"
    else
      echo -e ":: $1"
    fi
  fi
}

#
# Description:
#   Prints a white subsection message preceded by a green '  -> ' text to a
#   standard output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_subsection()
{
  if [[ $MODE -ge 2 ]]; then
    if [ -c /dev/stdout ]; then
      echo -e "\e[1;32m  -> \e[1;37m$1\e[0m"
    else
      echo -e "  -> $1"
    fi
  fi
}

#
# Description:
#   Prints a verbose message to a standard output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_verbose()
{
  if [[ $MODE -ge 3 ]]; then
    echo -e  "$1"
  fi
}

#
# Description:
#   Prints a debug message to a standard output stream.
# Parameters:
#   [STRING] A message.
# Return:
#   Nothing
#
print_debug()
{
  if [[ $MODE -ge 4 ]]; then
    echo -e "$1"
  fi
}

# End of script
