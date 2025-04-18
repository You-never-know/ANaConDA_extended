#!/bin/bash

#
# Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
#
# This file is part of libdie.
#
# libdie is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# libdie is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libdie. If not, see <http://www.gnu.org/licenses/>.
#

#
# Description:
#   A script
# Author:
#   Jan Fiedor
# Version:
#   0.5.0.2
# Created:
#   23.08.2011
# Last Update:
#   07.09.2011
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
#   Generates a C++ code of a table mapping DWARF tag numbers to strings
#   describing these tags (uses boost::map_list_of to initialise the table)
# Parameters:
#   [STRING] A file containing information about the DWARF tags (dwarf.h)
# Return:
#   Nothing
#
gen_tagtable()
{
  # Generate the definition of the table
  echo "std::map< unsigned short, const char* >"
  echo "  g_tagToStringTable = boost::assign::map_list_of"
  # Generate the (tag-number, tag-string) pairs
  cat $1 | grep -E '^#define DW_TAG_' | sed -r 's/^#define ([a-zA-Z0-9_]*)[ \t]*([0-9abcdefx]*).*$/    (\2, "\1")/g'
  # Generate the trailing ;
  echo "  ;"
}

#
# Description:
#   Generates a C++ code of a table mapping DWARF attribute numbers to strings
#   which describe them (uses boost::map_list_of to initialise the table)
# Parameters:
#   [STRING] A file containing information about the DWARF attributes (dwarf.h)
# Return:
#   Nothing
#
gen_attrtable()
{
  # Generate the definition of the table
  echo "std::map< unsigned short, const char* >"
  echo "  g_attrToStringTable = boost::assign::map_list_of"
  # Generate the (attribute-number, attribute-string) pairs
  cat $1 | grep -E '^#define DW_AT_' | sed -r 's/^#define ([a-zA-Z0-9_]*)[ \t]*([0-9abcdefx]*).*$/    (\2, "\1")/g'
  # Generate the trailing ;
  echo "  ;"
}

#
# Description:
#   Generates a C++ code of a table mapping DWARF location operations to strings
#   which describe them (uses boost::map_list_of to initialise the table)
# Parameters:
#   [STRING] A file containing information about the DWARF location operations
#     (dwarf.h)
# Return:
#   Nothing
#
gen_locoptable()
{
  # Generate the definition of the table
  echo "std::map< unsigned char, const char* >"
  echo "  g_locOpToStringTable = boost::assign::map_list_of"
  # Generate the (location-operation-number, location-operation-string) pairs
  cat $1 | grep -E '^#define DW_OP_' | sed -r 's/^#define ([a-zA-Z0-9_]*)[ \t]*([0-9abcdefx]*).*$/    (\2, "\1")/g'
  # Generate the trailing ;
  echo "  ;"
}

#
# Description:
#   Generates a C++ code of a table mapping DWARF attribute form numbers to
#   numbers representing attribute form classes (uses boost::map_list_of to
#   initialise the table)
# Parameters:
#   [STRING] A file containing information about the DWARF attribute forms
#     (dwarf.h)
# Return:
#   Nothing
#
gen_formmap()
{
  # Generate the DWARF attribute class numbers
  echo "const int ADDRESS = 1;"
  echo "const int BLOCK = 2;"
  echo "const int SIGNED_CONSTANT = 4;"
  echo "const int UNSIGNED_CONSTANT = 8;"
  echo "const int FLAG = 16;"
  echo "const int REFERENCE = 32;"
  echo "const int STRING = 64;"
  echo "const int INDIRECT = 128;"
  echo "const int UNKNOWN = 256;"
  echo ""
  # Generate the definition of the table
  echo "std::map< unsigned short, int >"
  echo "  g_attrFormToClassTable = boost::assign::map_list_of"
  # Generate the (form-number, class-number) pairs
  for form in `cat $1 | grep -E '^#define DW_FORM_' | sed -r 's/^#define ([a-zA-Z0-9_]*)[ \t]*([0-9abcdefx]*).*$/\1/g'`; do
    if [[ $form == *addr* ]]; then
      class="ADDRESS"
    elif [[ $form == *block* ]]; then
      class="BLOCK"
    elif [[ $form == *sdata* ]]; then
      class="SIGNED_CONSTANT"
    elif [[ $form == *data* ]]; then
      class="UNSIGNED_CONSTAN"
    elif [[ $form == *flag* ]]; then
      class="FLAG"
    elif [[ $form == *ref* ]]; then
      class="REFERENCE"
    elif [[ $form == *str* ]]; then
      class="STRING"
    elif [[ $form == *indirect* ]]; then
      class="INDIRECT"
    else
      class="UNKNOWN"
    fi

    echo "    ("$form", "$class")"
  done
  # Generate the trailing ;
  echo "  ;"
}

#
# Description:
#   Generates a C++ code of a table mapping DWARF location operation numbers to
#   numbers representing location operation types (uses boost::map_list_of to
#   initialise the table)
# Parameters:
#   [STRING] A file containing information about the DWARF location operations
#     (dwarf.h)
# Return:
#   Nothing
#
gen_locopmap()
{
  # Generate the DWARF location operation type numbers
  echo "const int NO_OPERAND = 1;"
  echo "const int SIGNED_CONSTANT = 2;"
  echo "const int UNSIGNED_CONSTANT = 4;"
  echo "const int ADDRESS = 8; // Unsigned constant"
  echo "const int REGISTER = 16; // Unsigned constant"
  echo "const int OFFSET = 32; // Signed constant"
  echo "const int REGISTER_AND_OFFSET = 64;"
  echo "const int STACK_INDEX = 128; // Unsigned constant"
  echo "const int SIZE = 256; // Unsigned constant"
  echo ""
  # Generate the definition of the table
  echo "std::map< unsigned char, int >"
  echo "  g_locOpToTypeTable = boost::assign::map_list_of"
  # Generate the (location-operation-number, location-operation-type) pairs
  for op in `cat $1 | grep -E '^#define DW_OP_' | sed -r 's/^#define ([a-zA-Z0-9_]*)[ \t]*([0-9abcdefx]*).*$/\1/g'`; do
    if [[ $op == *const*s* || $op == *skip* || $op == *bra* ]]; then
      type="SIGNED_CONSTANT"
    elif [[ $op == *const*u* || $op == *uconst* ]]; then
      type="UNSIGNED_CONSTANT"
    elif [[ $op == *addr* ]]; then
      type="ADDRESS"
    elif [[ $op == *bregx* ]]; then
      type="REGISTER_AND_OFFSET"
    elif [[ $op == *regx* ]]; then
      type="REGISTER"
    elif [[ $op == *breg* ]]; then
      type="OFFSET"
    elif [[ $op == *pick* ]]; then
      type="STACK_INDEX"
    elif [[ $op == *piece* || $op == *size* ]]; then
      type="SIZE"
    else
      type="NO_OPERAND"
    fi

    echo "    ("$op", "$type")"
  done
  # Generate the trailing ;
  echo "  ;"
}

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
  $0 [-h|--help] <switch> [<args>]

switches:
  gencode  Generates C++ code from some DWARF information

parameters:
  -h, --help  Prints this help
"
}

#
# Description:
#   Prints a usage of the gencode switch
# Parameters:
#   None
# Return:
#   Nothing
#
usage_gencode()
{
  echo -e "\
usage:
  $0 gencode [-h|--help] <switch> FILE

Generates C++ code from some DWARF information

  FILE        A file containing the DWARF information

switches:
  tagtable    Generates a table mapping DWARF tag numbers to strings describing
              these tags (uses boost::map_list_of to initialise the table), the
              FILE must be a file containing DWARF debugging information values
              in which the information about DWARF tags are stored (dwarf.h)
  attrtable   Generates a table mapping DWARF attribute numbers to strings which
              describe them (uses boost::map_list_of to initialise the table),
              the FILE must be a file containing DWARF debugging information
              values in which the information about DWARF attributes are stored
              (dwarf.h)
  locoptable  Generates a table mapping DWARF location operations to strings
              describing these operations (uses boost::map_list_of to initialise
              the table), the FILE must be a file containing DWARF debugging
              information values in which the information about DWARF location
              operations are stored (dwarf.h)
  formmap     Generates a table mapping DWARF attribute form numbers to numbers
              representing attribute form classes (uses boost::map_list_of to
              initialise the table), the FILE must be a file containing DWARF
              debugging information values in which the information about DWARF
              attribute forms tags are stored (dwarf.h)
  locopmap    Generates a table mapping DWARF location operation numbers to
              numbers representing internal location operation types (uses
              boost::map_list_of to initialise the table), the FILE must be a
              file containing DWARF debugging information values in which the
              information about DWARF location operations are stored (dwarf.h)

parameters:
  -h, --help  Prints this help
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
  "gencode") # Generate C++ code from some DWARF information
    case "$2" in
      "tagtable") # Generate a table mapping DWARF tag numbers to their strings
        if [ -z "$3" ]; then
          errmess "no file specified."
          exit 1
        fi

        gen_tagtable "$3"
        ;;
      "attrtable") # Generate a table mapping DWARF attribute nums to strings
        if [ -z "$3" ]; then
          errmess "no file specified."
          exit 1
        fi

        gen_attrtable "$3"
        ;;
      "locoptable") # Generate a table mapping DWARF operation nums to strings
        if [ -z "$3" ]; then
          errmess "no file specified."
          exit 1
        fi

        gen_locoptable "$3"
        ;;
      "formmap") # Generate a table mapping DWARF attribute forms to classes
        if [ -z "$3" ]; then
          errmess "no file specified."
          exit 1
        fi

        gen_formmap "$3"
        ;;
      "locopmap") # Generate a table mapping DWARF operation numbers to types
        if [ -z "$3" ]; then
          errmess "no file specified."
          exit 1
        fi

        gen_locopmap "$3"
        ;;
      *) # Unknown parameter
        usage_gencode
        exit 1
        ;;
    esac
    ;;
  *) # Unknown parameter
    usage
    exit 1
    ;;
esac

# End of script
