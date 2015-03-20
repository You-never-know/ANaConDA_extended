#!/bin/bash
#
# Description:
#   A script simplifying debugging the ANaConDA framework, its analysers and
#   the analysed programs with gdb (GNU debugger).
# Author:
#   Jan Fiedor
# Version:
#   1.0
# Created:
#   16.03.2015
# Last Update:
#   20.03.2015
#

# Get the information about the process we want to debug
while read line; do
  if [[ "$line" =~ "Pausing to attach to pid"* ]]; then
    # Get the PID of the process to which we need to attach the debugger
    line_as_array=($line)
    GDB_ATTACH_PID=${line_as_array[5]}
  fi
  # Skip the next line (no useful info there)
  read line
  # 3rd line contains information about the file we want to debug
  read line
  GDB_ADD_SYMBOL_FILE=$line
  # We got all the information needed to configure the debugger
  break
done

# Need to execute this commands to attach to the process
echo "attach "$GDB_ATTACH_PID > commands.gdb
echo $GDB_ADD_SYMBOL_FILE >> commands.gdb

# Run the debugger in a separate tab
konsole --new-tab -e "gdb -x `pwd`/commands.gdb"

# Discard all the remaining output from the framework, analyser or program
cat >/dev/null

# Clean the temporary files
rm -f commands.gdb

# End of script
