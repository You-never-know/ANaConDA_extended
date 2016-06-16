#!/bin/bash
#
# Description:
#    A script simplifying updating ANaConDA files on remote servers.
# Author:
#   Jan Fiedor
# Version:
#   4.0.1
# Created:
#   16.10.2013
# Last Update:
#   16.06.2016
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
     [--files { snapshot | git | tracked }]
     [--format { tar | tar.gz | zip }]
     <server> [<target> [<target> ...]]
  $0 --pull [<url>]

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
    3) tracked: current version of the files tracked by git.
  --format { tar | tar.gz | zip }
    Create a tar, tar.gz, or zip archive if publishing the target. By default,
    create a tar.gz archive on Linux and zip archive on Windows.
  --pull [<url>]
    Update itself instead of a remote server, i.e., update local files instead
    of the remote ones. The local files will be replaced (overwritten) by the
    files stored (in an archive) at the given URL. If no URL is specified, the
    PULL_URL variable will be used to locate the remote files.
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
#   Stores a list of files which should be present in a snapshot into a file.
#   The files that should be part of the snapshot are taken from the [files]
#   section of the target's configuration file.
# Parameters:
#   [STRING] A name of the file.
#   [PATH] A path to a configuration file.
# Output:
#   None
# Return:
#   Nothing
#
dump_snapshot_files()
{
  # Helper variables
  local file_name=$1
  local config_file=$2
  local directory=

  # Get the files to update
  local directories=$(get_directories "$config_file")
  local files=$(get_files "$config_file")

  # Dump all explicitly specified files into the output file
  echo "$files" > "./$file_name"

  # Find all files in the specified directories and add them to the file
  for directory in $directories; do
    find "./$directory" -type f >> "./$file_name"
  done
}

#
# Description:
#   Stores a list of files tracked by GIT (including submodules) into a file.
# Parameters:
#   [STRING] A name of the file.
# Output:
#   None
# Return:
#   Nothing
#
dump_tracked_files()
{
  # Helper variables
  local file_name=$1

  # Get a list of files in the main repository
  git ls-files > $file_name

  # Get a list of files for each submodule
  git submodule --quiet foreach "git ls-files | sed \"s#^#\$path/#g\" >> `pwd`/$file_name"
}

#
# Description:
#   Creates a directory containing the latest version (revision) of the target,
#   including its submodules.
# Parameters:
#   [STRING] A name of the directory.
# Output:
#   None
# Return:
#   Nothing
#
clone_git_with_submodules()
{
  # Helper variables
  local directory_name=$1
  local archive=

  # Create the specified directory
  mkdir -p $directory_name

  # Get the files of the HEAD revision as archive
  git archive --format=tar HEAD > $directory_name-head.tar

  # Get the files each submodule as archive
  git submodule --quiet foreach "git archive --format=tar --prefix=\$path/ \$sha1 > `pwd`/$directory_name-submodule-\$sha1.tar"

  # Extract all archives into the specified directory
  for archive in `find . -mindepth 1 -maxdepth 1 -type f -iname "$directory_name*tar"`; do
    tar --directory "./$directory_name" -xf $archive
  done

  # Delete the archives
  rm ./$directory_name-head.tar ./$directory_name-submodule*
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

  # Create a directory containing the latest version of the target
  clone_git_with_submodules "$archive_name"

  # Pack everything in this directory into a single archive and return its path
  archive_files "./$archive_name" "$archive_name" "$archive_format"

  # Delete the temporary directory
  rm -rf "./$archive_name"
}

#
# Description:
#   Creates an archive containing all files in a directory or from a list.
# Parameters:
#   [PATH]   A path to a directory or a file. If a directory is specified, all
#            files in this directory will be stored in the archive. If a file
#            is specified, it must contain paths to the files which should be
#            stored in the archive (each line must contain path to one file).
#   [STRING] A name of the archive. If no name is specified, the name of the
#            directory or file will be used as the name of the archive.
#   [STRING] A format of the archive (tar, tar.gz, zip). Default is tar.gz.
# Output:
#   A full path to the created archive.
# Return:
#   Nothing
#
archive_files()
{
  # Helper variables
  local path=$1
  local archive_name=$2
  local archive_format=$3

  # Generate a tar.gz archive if no format specified
  if [ -z "$archive_format" ]; then
    local archive_format="tar.gz"
  fi

  # Check if the format is supported, if not, use the defaults
  if ! [[ "$archive_format" =~ ^tar$|^tar\.gz$|^zip$ ]]; then
    terminate "archive_files: unknown archive format $archive_format."
  fi

  # Archive the files
  if [ -d "$path" ]; then
    # Archive all files in a directory
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
        terminate "archive_files: unknown archive format $archive_format."
        ;;
    esac
  elif [ -f "$path" ]; then
    # Archive files specified in a file
    case "$archive_format" in
      "tar")
        tar -cf "./$archive_name.$archive_format" --files-from "./$path"
        ;;
      "tar.gz")
        tar -zcf "./$archive_name.$archive_format" --files-from "./$path"
        ;;
      "zip")
        zip -q "./$archive_name.$archive_format" -@ < "./$path"
        ;;
      *)
        terminate "archive_files: unknown archive format $archive_format."
        ;;
    esac
  else
    # Invalid path
    terminate "archive_files: $path is not a file or a directory."
  fi

  # Return a full path to the create archive
  echo "$archive_name.$archive_format"
}

#
# Description:
#   Updates files on a remote server.
# Parameters:
#   [STRING] A path to a configuration file.
# Output:
#   None
# Return:
#   Nothing
#
update_target()
{
  # Helper variables
  local config_file=$1
  local target=`basename $config_file`

  # Skip the target if it is not in the list of targets to update
  if [ ! -z "$TARGETS" ]; then
    if ! list_contains "$TARGETS" "$target"; then
      return
    fi
  fi

  print_section "Updating target $target"

  print_subsection "resolving source directory on the local server"

  # Get the local directory
  local local_dir=$(get_local_dir "$config_file")

  if [ ! -d "$local_dir" ]; then
    print_warning "local directory $local_dir not found, ignoring target."
    return
  fi

  print_info "     $local_dir"

  print_subsection "resolving target directory on the remote server"

  # Get the remote directory
  if [ "$PUBLISH" == "1" ]; then
    local remote_dir=$(get_publish_dir "$config_file" "$SERVER_INFO")
  else
    local remote_dir=$(get_remote_dir "$config_file" "$SERVER_INFO")
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

  # The paths to the files to update are relative to this directory
  cd $local_dir

  if [ "$PUBLISH" == "1" ]; then
    # Publish an archive containing the files
    if [ "$UPDATE_TYPE" == "git" ]; then
      # Latest revision of files tracked by GIT
      local archive_name="$target-git-`git rev-parse --short HEAD`"

      # Create an archive containing the latest GIT revision
      local archive=$(archive_git_with_submodules "$archive_name" "$FORMAT")
    else
      # Snapshot of the files specified in the configuration file or current
      # version of files tracked by GIT (same code but different file lists)
      local archive_name="$target-$UPDATE_TYPE-`date --utc +"%Y%m%dT%H%M"`"
      local file_list="$archive_name.filelist"

      # Get a list of files that should be archived
      dump_${UPDATE_TYPE}_files "./$file_list" "$config_file"

      # Create an archive containing these files
      local archive=$(archive_files "./$file_list" "$archive_name" "$FORMAT")

      rm "./$file_list"
    fi

    if [ ! -f "./$archive" ]; then
      print_warning "failed to create archive, ignoring target."
      return
    fi

    rsync -v -R -e "ssh -p $PORT" "$archive" $USER@$HOSTNAME:$remote_dir

    # Flag the archive as the latest version (create a symlink for it)
    ssh $USER@$HOSTNAME -p $PORT "cd $remote_dir; ln -sf ./$archive ./$target-$UPDATE_TYPE-latest.$FORMAT"

    rm "./$archive"
  else
    # Update the files
    if [ "$UPDATE_TYPE" == "git" ]; then
      # Latest revision of files tracked by GIT
      local workdir="$target-git-`git rev-parse --short HEAD`"

      clone_git_with_submodules "$workdir"

      cd "./$workdir"
      rsync -v -R -r -e "ssh -p $PORT" "./" $USER@$HOSTNAME:$remote_dir
      cd ..

      rm -rf "./$workdir"
    else
      # Snapshot of the files specified in the configuration file or current
      # version of files tracked by GIT (same code but different file lists)
      local file_list="$archive_name.filelist"

      # Get a list of files that should be updated
      dump_${UPDATE_TYPE}_files "./$file_list" "$config_file"

      # Update all files in the list
      rsync -v -R -e "ssh -p $PORT" --files-from="./$file_list" "./" $USER@$HOSTNAME:$remote_dir

      rm "./$file_list"
    fi
  fi

  # Move back the the directory where we executed the script
  cd $SCRIPT_DIR
}

#
# Description:
#   Updates files on a local server.
# Parameters:
#   [STRING] A URL of an archive containing the new versions of the files.
# Output:
#   None
# Return:
#   Nothing
#
update_local()
{
  # Helper variables
  local url=$1

  print_section "Updating local server"

  print_subsection "downloading files from the remote server"

  # Download the archive containing the newer versions of files
  wget --passive-ftp --no-check-certificate -c $url 2>&1 &> /dev/null

  if [ $? != 0 ]; then
    terminate "could not download files from the remote server."
  fi

  print_subsection "updating local files"

  # Get the name of the archive from the URL
  local archive=`echo $url | sed -e "s/^.*\/\([^\/]*\)$/\1/"`

  # Replace the local files with the files within the archive
  tar -xf $archive

  # Remove temporary files
  rm -rf $archive
}

# Program section
# ---------------

# Default values for optional parameters
BASH_INVOCATION_ARGS=
UPDATE_TYPE=snapshot
PUBLISH=0

# Default archive format differs for Windows and Linux
if [ `uname -o` == "Cygwin" ]; then
  FORMAT=zip
else
  FORMAT=tar.gz
fi

# Initialize environment first, optional parameters might override the values
env_init

# Check in we are not operating in the pull mode (updating ourselves)
if [ "$1" == "--pull" ]; then
  if [ ! -z "$2" ]; then
    PULL_URL=$2
  fi

  # Update local files with the ones in the archive at the given URL
  update_local $PULL_URL

  # The pull mode ends its work here
  exit 0
fi

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
      if ! [[ "$2" =~ ^snapshot$|^git$|^tracked$ ]]; then
        terminate "files to update must be snapshot, git, or tracked."
      fi
      UPDATE_TYPE=$2
      shift
      ;;
    "--format")
      if [ -z "$2" ]; then
        terminate "missing specification of archive format."
      fi
      if ! [[ "$2" =~ ^tar$|^tar\.gz$|^zip$ ]]; then
        terminate "archive format must be tar, tar.gz, or zip."
      fi
      FORMAT=$2
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
