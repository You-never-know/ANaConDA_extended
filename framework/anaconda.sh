#!/bin/bash

#
# Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
#    A script simplifying the usage of ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   0.2
# Created:
#   30.06.2012
# Last Update:
#   22.03.2013
#

# Helper functions section
# ------------------------

#
# Description:
#   Prints an error message with preceding red 'error: ' to a console and
#   normal message with preceding 'error: ' otherwise
# Parameters:
#   [STRING] A message
# Return:
#   Nothing
#
errmess()
{
  if [ -c /dev/stderr ]; then
    echo -e "\e[1;31merror: \e[0m$1" 1>&2
  else
    echo "error: "$1 1>&2
  fi
}

# Functions section
# -----------------

#
# Description:
#   Prints a script usage
# Parameters:
#   None
# Return:
#   Nothing
#
usage()
{
  echo -e "\
usage:
  $0 <analyser> <program> [<arguments>]

  <analyser>  A path to the analyser to be used.
  <program>   A path to the program to be analysed.
  <arguments> A space-separated list of arguments passed to the program.
"
}

# Program section
# ---------------

# Check parameters
case "$1" in
  "-h"|"--help")
    usage
    exit 0
    ;;
  "")
    errmess "no analyser specified."
    echo ""
    usage
    exit 1
    ;;
esac

if [ -z "$2" ]; then
  errmess "no program specified."
  echo ""
  usage
  exit 1
fi

if [ ! -f "$PIN_HOME/pin" ]; then
  if [ -z "$PIN_HOME" ]; then
    errmess "pin.bat not found, the PIN_HOME variable is not set or empty, set it to point to the installation directory of PIN."
    exit 1
  fi
  if [ ! -d "$PIN_HOME" ]; then
    errmess "error: pin.bat not found, the PIN_HOME variable do not point to a directory, set it to point to the installation directory of PIN."
    exit 1
  fi
  errmess "pin.bat not found, set the PIN_HOME variable to point to the installation directory of PIN."
  exit 1
fi

if [ ! -f "$ANACONDA_HOME/lib/intel64/anaconda.so" ]; then
  if [ -f "`pwd`/lib/intel64/anaconda.so" ]; then
    ANACONDA_HOME=`pwd`
  elif [ -z "$ANACONDA_HOME" ]; then
    errmess "anaconda.so not found, the ANACONDA_HOME variable is not set or empty, set it to point to the installation directory of ANaConDA."
    exit 1
  else
    errmess "anaconda.so not found, set the ANACONDA_HOME variable to point to the installation directory of ANaConDA."
    exit 1
  fi
fi

ANALYSER=$1
shift

"$PIN_HOME/pin" -t "$ANACONDA_HOME/lib/intel64/anaconda" --show-settings -a $ANALYSER -- $*

# End of script
