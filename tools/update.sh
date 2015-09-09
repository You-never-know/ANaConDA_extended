#!/bin/bash
#
# Description:
#    A script simplifying updating ANaConDA files on remote servers.
# Author:
#   Jan Fiedor
# Version:
#   2.5
# Created:
#   16.10.2013
# Last Update:
#   09.09.2015
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
  $0 [--help] [--interactive] [--publish] [--publish-dir <path>]
     [--local-source-dir <path>] [--remote-source-dir <path>]
     [--files { snapshot | git }]
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
  --publish
    Publish the target instead on the remote server instead of updating it. The
    main difference is that publishing will pack the target into an archive and
    then upload the archive to the publishing folder.
  --publish-dir <path>
    A path to a remote directory where the published target should be uploaded.
    Overrides the PUBLISH_DIR variable on the remote server.
  --local-source-dir <path>
    A path to a local directory contaning the source files to update. Overrides
    the SOURCE_DIR variable on the local computer.
  --remote-source-dir <path>
    A path to a remote directory contaning the source files to update. Replaces
    the SOURCE_DIR variable on the remote server.
  --files { snapshot | git }
    Update the following files (default is snapshot):
    1) snapshot: current version of the files specified in the target's
       configuration file (in the files section).
    2) git: latest revision of the files tracked by git.
"
}

#
# Description:
#   Evaluates a path on a remote server.
# Parameters:
#   [STRING] A path to be evaluated on the remote server. The path may contain
#            variables, however, do not forget to escape them or they would be
#            evaluated on the local computer and not at the remote server! The
#            string passed to the functions must contain the dolars ($) before
#            before the names of the variables which must be evaluated on the
#            remote server!
#   [ARRAY]  An array contaning username, servername and port, respectively.
#   [STRING] A semicolon-separated list variables which must to be defined on
#            the remote server before performing the evaluation. Do not forget
#            to escape any variables in the value section if they need to be
#            evaluated on the remote server! Example: "var1=text;var2=\$HOME".
# Output:
#   [STRING] A path evaluated on the remote server.
# Return:
#   Nothing
#
evaluate_remote_path()
{
  # Helper variables
  local path=$1
  local server_info=$2
  local variable_definitions=$3

  # Extract the username, servername and port
  read user hostname port <<<$(echo "$server_info")

  # Evaluate the path on the remote server, escape variables ($) before sending
  local path_escaped=$(echo $path | sed 's/\$/\\\$/g')
  local path_evaluated=`ssh $user@$hostname -p $port bash $BASH_INVOCATION_ARGS -c "\"source ~/.anaconda/environment; $variable_definitions; echo EVALUATED_PATH=$path_escaped\" | grep 'EVALUATED_PATH=' | sed 's/^EVALUATED_PATH=//'" 2>/dev/null`

  # Return the path evaluated on a remote server
  echo "$path_evaluated"
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
#   [STRING] A path to the file contaning information about the path to the
#            remote directory.
#   [ARRAY]  An array contaning username, servername and port, respectively.
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

  # Remote directory is in the remote section
  local remote_dir=$(get_section $file_path "remote")

  # Evaluate the remote directory on the remote server
  evaluate_remote_path "$remote_dir" "$server_info" "$REPLACE_REMOTE_SOURCE_DIR_COMMAND TARGET=$target"
}

#
# Description:
#   Gets a path to the publish directory.
# Parameters:
#   [STRING] A path to the file contaning information about the path to the
#            publish directory.
#   [ARRAY]  An array contaning username, servername and port, respectively.
# Output:
#   [STRING] A path to the publish directory.
# Return:
#   Nothing
#
get_publish_dir()
{
  # Helper variables
  local file_path=$1
  local server_info=$2

  # Remote directory is in the remote section
  local publish_dir=$(get_section $file_path "publish")

  # Evaluate the remote directory on the remote server
  evaluate_remote_path "$publish_dir" "$server_info" "$REPLACE_PUBLISH_DIR_COMMAND TARGET=$target"
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
#   Creates an archive containing the latest version (revision) of the target,
#   including its submodules.
# Parameters:
#   [STRING] A name of the archive.
#   [STRING] A format of the archive (tar, tar.gz, zip). Default is tar.gz.
# Output:
#   A full path to the created archive.
# Return:
#   Nothing
#
archive_git_with_submodules()
{
  # Helper variables
  local archive_name=$1
  local archive_format=$2
  local archive=

  # Generate a tar.gz archive if no format specified
  if [ -z "$archive_format" ]; then
    local archive_format="tar.gz"
  fi

  # Check if the format is supported
  if ! [[ "$archive_format" =~ ^tar$|^tar\.gz$|^zip$ ]]; then
    print_error "cannot create $archive_format archive, only tar, tar.gz and zip are supported."
    return
  fi

  # Archive the HEAD revision
  git archive --format=tar HEAD > $archive_name-head.tar

  # Archive each submodule
  git submodule --quiet foreach "git archive --format=tar --prefix=\$path/ \$sha1 > `pwd`/$archive_name-submodule-\$sha1.tar"

  # Merge all archives into one
  mkdir -p $archive_name

  # First extract the all archived files into a temporary direcotry
  for archive in `find . -mindepth 1 -maxdepth 1 -type f -iname "$archive_name*tar"`; do
    tar --directory "./$archive_name" -xf $archive
  done

  # Then pack everything in the temporary directory into a single archive
  case "$archive_format" in
    "tar")
      tar --directory "./$archive_name" -cf $archive_name.$archive_format .
      ;;
    "tar.gz")
      tar --directory "./$archive_name" -zcf $archive_name.$archive_format .
      ;;
    "zip")
      cd "./$archive_name" && zip -r -q ../$archive_name . && cd .. 
      ;;
    *)
      terminate "unknown archive format $archive_format."
      ;;
  esac

  # Cleanup the temporary directories and archives
  rm -rf ./$archive_name ./$archive_name-head.tar ./$archive_name-submodule*

  # Return the name of the archive
  echo $archive_name.$archive_format
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

  if [ ! -d "$local_dir" ]; then
    print_warning "local directory $local_dir not found, ignoring target."
    return
  fi

  print_info "     $local_dir"

  print_subsection "resolving target directory on the remote server"

  # Get the remote directory
  if [ "$PUBLISH" == "1" ]; then
    local remote_dir=$(get_publish_dir "$file_path" "$SERVER_INFO")
  else
    local remote_dir=$(get_remote_dir "$file_path" "$SERVER_INFO")
  fi

  if [ -z "$remote_dir" ]; then
    print_warning "remote directory is empty, ignoring target."
    return
  fi

  if [ `ssh $USER@$HOSTNAME -p $PORT bash $BASH_INVOCATION_ARGS -c "\"mkdir -p $remote_dir &>/dev/null; echo RESULT=\$?\" | grep 'RESULT=' | sed 's/^RESULT=//'" 2>/dev/null` == "1" ]; then
    print_warning "remote directory $remote_dir not found and cannot be created, ignoring target."
    return
  fi

  print_info "     $remote_dir"

  if [ "$PUBLISH" == "1" ]; then
    print_subsection "publishing target"
  else
    print_subsection "updating files"
  fi

  # Get the files to update
  local directories=$(get_directories "$file_path")
  local files=$(get_files "$file_path")

  # The paths to the files to update are relative to this directory
  cd $local_dir

  if [ "$PUBLISH" == "1" ]; then
    # Publish an archive containing the files
    if [ "$UPDATE_TYPE" == "snapshot" ]; then
      # Snapshot of the files specified in the configuration file
      local archive="$target-snapshot-`date --utc +"%Y%m%d%H%M"`.tar.gz"
      tar -zcvf $archive $directories $files
    else
      # Latest revision of files tracked by GIT
      local archive=$(archive_git_with_submodules "$target-git-`git rev-parse --short HEAD`")

      if [ ! -f "$archive" ]; then
        print_warning "failed to get the latest GIT revision, ignoring target."
        return
      fi
    fi

    rsync -v -R -e "ssh -p $PORT" $archive $USER@$HOSTNAME:$remote_dir

    rm $archive
  else
    # Update the files
    if [ "$UPDATE_TYPE" == "snapshot" ]; then
      # Snapshot of the files specified in the configuration file
      rsync -v -R -r -e "ssh -p $PORT" $directories $USER@$HOSTNAME:$remote_dir
      rsync -v -R -e "ssh -p $PORT" $files $USER@$HOSTNAME:$remote_dir
    else
      # Latest revision of files tracked by GIT
      local workdir="$target-git-`git rev-parse --short HEAD`"
      local archive="$workdir.tar.gz"
      git archive --format=tar.gz --prefix="$workdir/" HEAD > $archive

      tar -xf ./$archive

      cd ./$workdir
      rsync -v -R -r -e "ssh -p $PORT" "./" $USER@$HOSTNAME:$remote_dir
      cd ..

      rm -rf ./$workdir
      rm $archive
    fi
  fi

  # Move back the the directory where we executed the script
  cd $SCRIPT_DIR
}

# Program section
# ---------------

# Default values for optional parameters
BASH_INVOCATION_ARGS=
UPDATE_TYPE=snapshot
PUBLISH=0

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
    "--publish")
      PUBLISH=1
      ;;
    "--publish-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the publish directory."
      fi
      REPLACE_PUBLISH_DIR_COMMAND="PUBLISH_DIR=$2;"
      shift
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
    "--files")
      if [ -z "$2" ]; then
        terminate "missing specification of files to update."
      fi
      if ! [[ "$2" =~ ^snapshot|git$ ]]; then
        terminate "files to update must be snapshot or git."
      fi
      UPDATE_TYPE=$2
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
