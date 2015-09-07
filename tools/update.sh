#!/bin/bash
#
# Description:
#    A script simplifying updating ANaConDA files on remote servers.
# Author:
#   Jan Fiedor
# Version:
#   1.4
# Created:
#   16.10.2013
# Last Update:
#   07.09.2015
#

# Search the folder containing the script for the included scripts
PATH=$PATH:$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Include required scripts
source utils.sh

# Settings section
# ----------------

# Directory containing information about files
FILES_DIR="files"

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
  $0 [--help] [--interactive] [--local-source-dir] [--remote-source-dir]
     <server> [<target> [<target> ...]]

required arguments:
  <server>  An identification of a remote server. Might be a name of the server
            in the ssh configuration file (~/.ssh/config) or a string in the
            '[<username>@]<servername>[:<port>]' format. If 'username' is not
            specified, the name of the currently logged user will be used. If
            'port' is not specified, the default ssh port (22) will be used.

positional arguments:
  <target>  A name of a target to update. A name of a file with the information
            about the files to update. If no target is specified, all targets
            will be updated.

optional arguments:
  --help
    Print the script usage.
  --interactive
    Access the remote server interactively, i.e., use an interactive login bash
    shell to connect to the remote server. Use this option when the environment
    variables used in the specifications of remote diretories are not evaluated
    correctly. The remote server's response will propably be a much slower, but
    the environment variables should be set correctly.
  --local-source-dir
    A path to a local directory contaning the source files to update. Overrides
    the SOURCE_DIR variable on the local computer.
  --remote-source-dir
    A path to a remote directory contaning the source files to update. Replaces
    the SOURCE_DIR variable on the remote server.
"
}

#
# Description:
#   Gets information needed to connect to a remote server.
# Parameters:
#   [STRING] A name of the server in the ssh configuration file or a string in
#            the '[<username>@]<servername>[:<port>]' format. If 'username' is
#            not specified, name of the currently logged user will be used. If
#            'port' is not specified, default ssh port (22) will be used.
# Output:
#   [ARRAY] An array contaning username, servername and port, respectively.
# Return:
#   Nothing
#
get_server_info()
{
  # Helper variables
  local server_info=$1
  local state=search

  # Try to find server information in the ssh configuration file
  while read line || [[ -n "$line" ]]; do
    case "$state" in
      "search")
        if [[ $line =~ ^Host\ .* ]]; then
          local aliases=(${line:5})
          for alias in ${aliases[@]}; do
            if [ "$alias" == "$server_info" ]; then
              state=extract
              break
            fi
          done
        fi
        ;;
      "extract")
        if [[ $line =~ ^[\ ]*HostName\ .* ]]; then
          local hostname_entry=($line)
          local hostname=${hostname_entry[1]}
        elif [[ $line =~ ^[\ ]*Port\ .* ]]; then
          local port_entry=($line)
          local port=${port_entry[1]}
        elif [[ $line =~ ^[\ ]*User\ .* ]]; then
          local user_entry=($line)
          local user=${user_entry[1]}
        elif [[ $line =~ ^Host\ .* ]]; then
          break
        fi
        ;;
    esac
  done < ~/.ssh/config

  if [ "$state" == "search" ]; then
    # Server not found in the ~/.ssh/config file, maybe the user specified
    # the server directly using a URL in [<user>@]<server>[:<port>] format
    # (1) transform [<user>@]<server>[:<port>] into <user>|<server>|<port>
    # (2) if <user> not present, prepend the name of currently logged user
    # (3) if <port> not present, append the default port for SSH (port 22)
    # (4) transform <user>|<server>|<port> to the '<user> <server> <port>'
    # Note: if the input ([<user>@]<server>[:<port>] string) is invalid,
    #   the result will be an empty string.
    read user hostname port <<<$(echo $server_info | sed -n 's/^\(\([A-Za-z0-9._]\+\)@\)\?\([A-Za-z0-9._]\+\)\(:\([0-9]\+\)\)\?$/\2|\3|\5/p' | sed "s/^|/`whoami`|/" | sed 's/|$/|22/' | sed 's/|/ /g')

    if [ -z "$hostname" ]; then
      print_error "invalid server specification $server_info, must be in format [<username>@]<servername>[:<port>]."
      exit 1
    fi
  else
    if [ -z "$hostname" ]; then
      print_error "no hostname for server $server_info found in the ~/.ssh/config file."
      exit 1
    fi

    if [ -z "$user" ]; then
      user=`whoami`
    fi

    if [ -z "$port" ]; then
      port=22
    fi
  fi

  echo $user $hostname $port
}

#
# Description:
#   Gets information stored in a specific section of a file.
# Parameters:
#   [STRING] A path to the file.
#   [STRING] A name of the section.
# Output:
#   [STRING] Information stored in the specified section of the given file.
# Return:
#   Nothing
#
get_section()
{
  # Helper variables
  local file_path=$1
  local section_name=$2
  local section_info=
  local state=search

  # Find and extract information from the specified section
  while read line || [[ -n "$line" ]]; do
    case "$state" in
      "search")
        if [ "$line" == "[$section_name]" ]; then
          state=extract
        fi
        ;;
      "extract")
        if [[ $line =~ ^[[].* ]]; then
          break
        else
          section_info="$section_info$line\n"
        fi
        ;;
    esac
  done < $file_path

  if [ "$state" == "search" ]; then
    print_error "section $section_name not found in file $file_path."
    exit 1
  fi

  echo -e "$section_info"
}

#
# Description:
#   Gets a path to the local directory.
# Parameters:
#   [STRING] A path to the file contaning information about the local directory.
# Output:
#   [STRING] A path to the local directory.
# Return:
#   Nothing
#
get_local_dir()
{
  # Helper variables
  local file_path=$1

  # Local directory is in the local section
  local local_dir=$(get_section $file_path "local")

  # Evaluate the path, it might contain variables
  eval "echo $local_dir"
}

#
# Description:
#   Gets a path to the remote directory.
# Parameters:
#   [STRING] A path to the file contaning information about the remote directory.
#   [ARRAY] An array contaning username, servername and port, respectively.
# Output:
#   [STRING] A path to the remote directory.
# Return:
#   Nothing
#
get_remote_dir()
{
  # Helper variables
  local file_path=$1
  local server_info=$2
  read user hostname port <<<$(echo "$server_info")

  # Remote directory is in the remote section
  local remote_dir=$(get_section $file_path "remote")

  # Evaluate the path on the remote server, escape variables ($) before sending
  local remote_dir_escaped=$(echo $remote_dir | sed 's/\$/\\\$/g')
  local remote_dir_evaluated=`ssh $user@$hostname -p $port bash $BASH_INVOCATION_ARGS -c "\"source ~/.anaconda/environment; $REPLACE_REMOTE_SOURCE_DIR_COMMAND TARGET=$TARGET; echo REMOTE_DIR=$remote_dir_escaped\" | grep 'REMOTE_DIR=' | sed 's/^REMOTE_DIR=//'" 2>/dev/null`
  echo "$remote_dir_evaluated"
}

#
# Description:
#   Gets a list of directories which should be updated.
# Parameters:
#   [STRING] A path to the file contaning the paths to the directories.
# Output:
#   [ARRAY] A list of directories which should be updated.
# Return:
#   Nothing
#
get_directories()
{
  # Helper variables
  local file_path=$1

  # Directories are in the files section together with files
  local all_files=$(get_section $file_path "files")

  # Get only the directories
  local directories=`echo "$all_files" | grep -E '^.*\/\*\*$' | sed -e 's/\/\*\*$/\//g'`
  echo "$directories"
}

#
# Description:
#   Gets a list of files which should be updated.
# Parameters:
#   [STRING] A path to the file contaning the paths to the files
# Output:
#   [ARRAY] A list of files which should be updated.
# Return:
#   Nothing
#
get_files()
{
  # Helper variables
  local file_path=$1

  # Files are in the files section together with directories
  local all_files=$(get_section $file_path "files")

  # Get only the files
  local files=`echo "$all_files" | grep -v -E '^.*\/\*\*$' | sed -e 's/\/\*$/\//g'`
  echo "$files"
}

#
# Description:
#   Updates files on a remote server.
# Parameters:
#   [STRING] A path to a file contaning the paths to the files to update.
# Output:
#   None
# Return:
#   Nothing
#
update_target()
{
  # Helper variables
  local file_path=$1
  local target=`basename $file_path`

  # Skip the target if it is not in the list of targets to update
  if [ ! -z "$TARGETS" ]; then
    if ! list_contains "$TARGETS" "$target"; then
      return
    fi
  fi

  print_section "Updating target $target"

  print_subsection "resolving source directory on the local server"

  # Get the local directory
  local local_dir=$(get_local_dir "$file_path")

  print_info "     $local_dir"

  if [ ! -d $local_dir ]; then
    print_warning "local directory $local_dir not found, ignoring target."
    return
  fi

  print_subsection "resolving target directory on the remote server"

  # Get the remote directory
  local remote_dir=$(get_remote_dir "$file_path" "$SERVER_INFO")

  print_info "     $remote_dir"

  if [ `ssh $USER@$HOSTNAME -p $PORT bash $BASH_INVOCATION_ARGS -c "\"mkdir -p $remote_dir &>/dev/null; echo RESULT=\$?\" | grep 'RESULT=' | sed 's/^RESULT=//'" 2>/dev/null` == "1" ]; then
    print_warning "remote directory $remote_dir not found and cannot be created, ignoring target."
    return
  fi

  print_subsection "updating files"

  # Get the files to update
  local directories=$(get_directories "$file_path")
  local files=$(get_files "$file_path")

  # The paths to the files to update are relative to this directory
  cd $local_dir

  # Update the files
  rsync -v -R -r -e "ssh -p $PORT" $directories $USER@$HOSTNAME:$REMOTE_DIR
  rsync -v -R -e "ssh -p $PORT" $files $USER@$HOSTNAME:$REMOTE_DIR

  # Move back the the directory where we executed the script
  cd $SCRIPT_DIR
}

# Program section
# ---------------

# Default values for optional parameters
BASH_INVOCATION_ARGS=

# Initialize environment first, optional parameters might override the values
env_init

# Process the optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--interactive")
      BASH_INVOCATION_ARGS=-li
      ;;
    "--local-source-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the local source directory."
      fi
      SOURCE_DIR=$2
      shift
      ;;
    "--remote-source-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the remote source directory."
      fi
      REPLACE_REMOTE_SOURCE_DIR_COMMAND="SOURCE_DIR=$2;"
      shift
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Check the required arguments
if [ -z "$1" ]; then
  terminate "no server specified."
fi

# Get the server identification
SERVER=$1

# Move to the positional arguments
shift

# Get the list of targets to update
TARGETS="$*"

# Find out how to connect to the remote server
SERVER_INFO=$(get_server_info "$SERVER")
read USER HOSTNAME PORT <<<$(echo "$SERVER_INFO")

# Update the files on the remote server
process_config_dir $FILES_DIR update_target

# End of script
