#!/bin/bash

#
# Copyright (C) 2010-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
#   Functions for formating and printing arbitrary messages.
# Author:
#   Jan Fiedor
# Version:
#   2.4
# Created:
#   26.03.2010
# Last Update:
#   13.10.2016
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
      echo -e "\033[1;31merror: \033[0m$1" 1>&2
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
      echo -e "\033[1;33mwarning: \033[0m$1"
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
#   [STRING] Additional parameters passed to the echo command.
# Return:
#   Nothing
#
print_info()
{
  if [[ $MODE -ge 2 ]]; then
    echo -e $2 "$1"
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
      echo -e "\033[1;34m:: \033[1;37m$1\033[0m"
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
      echo -e "\033[1;32m  -> \033[1;37m$1\033[0m"
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
#   [STRING] Additional parameters passed to the echo command.
# Return:
#   Nothing
#
print_verbose()
{
  if [[ $MODE -ge 3 ]]; then
    echo -e $2 "$1"
  fi
}

#
# Description:
#   Prints a debug message to a standard output stream.
# Parameters:
#   [STRING] A message.
#   [STRING] Additional parameters passed to the echo command.
# Return:
#   Nothing
#
print_debug()
{
  if [[ $MODE -ge 4 ]]; then
    echo -e $2 "$1"
  fi
}

# End of script
