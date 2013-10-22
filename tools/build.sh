#!/bin/bash
#
# Description:
#   A script simplifying building ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   0.1
# Created:
#   18.10.2013
# Last Update:
#   22.10.2013
#

source messages.sh

# Settings section
# ----------------

# CMake information
CMAKE_STABLE_VERSION=2.8.12
CMAKE_STABLE_DIR="cmake-$CMAKE_STABLE_VERSION"
CMAKE_STABLE_TGZ="$CMAKE_STABLE_DIR.tar.gz"
CMAKE_STABLE_URL="http://www.cmake.org/files/v${CMAKE_STABLE_VERSION:0:3}/$CMAKE_STABLE_TGZ"

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
  $0 [--help] [--build-type { release | debug }] [--prefix]
     [--check-environment] [--setup-environment] <target>

required arguments:
  <target>  A target to build. Might be a required library, the framework itself
            or a specific analyser.

optional arguments:
  --help
    Print the script usage.
  --build-type { release | debug }
    Build the release or debug version of the target, respectively. Default is
    to build the release version.
  --prefix
    A path to a directory which will serve as an installation root.
  --check-environment
    Check if the tools necessary for building ANaConDA are available.
  --setup-environment
    Setup the environment to be able to build ANaConDA (e.g. installs all the
    tools needed to build ANaConDA which are not available).
"
}

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
#   Checks if a specific CMake binary meets the version requirements.
# Parameters:
#   [STRING] A path to the CMake binary.
#   [STRING] A name of the variable to which the version of the CMake binary
#            should be stored. If specified, the version will not be printed
#            to the output.
# Output:
#   The version of the CMake binary if no variable to which it should be stored
#   was specified.
# Return:
#   0 if the CMake binary meets the version requirements, 1 otherwise.
#
check_cmake_version()
{
  # Get the version of the specified CMake binary
  local version_of_checked_binary=`$1 --version 2>&1 | grep "cmake version" | grep -o -E "[0-9.]+"`

  if [ "$version_of_checked_binary" != "" ]; then
    # Some version of CMake is present, but we need version 2.8.3 or greater
    local version_parts=( ${version_of_checked_binary//./ } )

    if [ -z "$2" ]; then
      echo "$version_of_checked_binary"
    else
      eval $2="'$version_of_checked_binary'"
    fi

    if [ ${version_parts[0]} -lt 2 -o ${version_parts[1]} -lt 8 -o ${version_parts[2]} -lt 3 ]; then
      # Old version of CMake, cannot use this version
      return 1
    else
      # Version 2.8.3 or greater, can use this version
      return 0
    fi
  else
    # Binary not found or could not get version
    if [ -z "$2" ]; then
      echo "unknown"
    else
      eval $2="'unknown'"
    fi

    return 1
  fi
}

#
# Description:
#   Checks if there exist a CMake binary that meets the version requirements.
# Parameters:
#   [STRING] A name of the variable to which the path to the CMake binary which
#            meets the version requirement will be stored.
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable CMake binary was found, 1 otherwise.
#
check_cmake()
{
  # Helper variables
  local cmake_version
  local index

  # List of CMake binaries to check together with their description
  local cmake_binaries=("$CMAKE" "cmake" "$INSTALLATION_PREFIX/bin/cmake")
  local cmake_binaries_desc=("\$CMAKE variable" "default cmake" "local installation")

  print_subsection "checking CMake build system"

  # Try to find a version of CMake which we can use the build the ANaConDA
  for index in ${!cmake_binaries[@]}; do
    print_info "     checking ${cmake_binaries_desc[$index]}... " -n

    if check_cmake_version "${cmake_binaries[$index]}" cmake_version; then
      # Suitable version found, save path to it (if requested) and return
      print_info "success, version $cmake_version"

      if [ ! -z "$1" ]; then
        eval $1="'${cmake_binaries[$index]}'"
      fi

      return 0
    else
      # Cannot use this version, continue the search
      print_info "fail, version $cmake_version"
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds CMake from its sources.
# Parameters:
#   None
# Output:
#   An error message if the build fails.
# Return:
#   Nothing
#
build_cmake()
{
  # Download the archive containing the cmake source code
  ${DOWNLOAD_COMMAND//%u/$CMAKE_STABLE_URL}
  # Extract the source code
  tar xvf ./$CMAKE_STABLE_TGZ
  # Compile the source code
  cd $CMAKE_STABLE_DIR
  ./bootstrap --prefix=$INSTALLATION_PREFIX && make && make install || terminate "cannot build CMake."
  cd ..
}

# Program section
# ---------------

# Save information about the directory in which we executed the script
SCRIPT_DIR=`pwd`

# Default values for optional parameters
BUILD_TYPE=release
INSTALLATION_PREFIX=$SCRIPT_DIR
PREBUILD_ACTION=none

# Process the optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--build-type")
      if [ -z "$2" ]; then
        terminate "missing build type."
      fi
      if ! [[ "$2" =~ ^release|debug$ ]]; then
        terminate "build type must be release or debug."
      fi
      BUILD_TYPE=$2
      shift
      ;;
    "--prefix")
      if [ -z "$2" ]; then
        terminate "missing prefix."
      fi
      INSTALLATION_PREFIX=$2
      shift
      ;;
    "--check-environment")
      PREBUILD_ACTION=check
      ;;
    "--setup-environment")
      PREBUILD_ACTION=setup
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Process the required parameters
if [ -z "$1" ]; then
  terminate "no target specified."
else
  BUILD_TARGET=$1
fi

print_section "Configuring build script..."

print_info "  determining download command... " -n

# Setup the command used to download files from the internet or local network
if [ -z "$DOWNLOAD_COMMAND" ]; then
  # If no download command is set, check if wget is available
  WGET_VERSION=`wget -V 2>&1 | grep -o -E "Wget.[0-9.]+"`

  if [ "$WGET_VERSION" != "" ]; then
    DOWNLOAD_COMMAND="wget --passive-ftp -c %u"
  fi
fi

print_info "$DOWNLOAD_COMMAND"

cd $INSTALLATION_PREFIX

# Execute all requested prebuild actions
if [ "$PREBUILD_ACTION" == "setup" ]; then
  print_section "Setting up build environment..."

  if ! check_cmake CMAKE; then
    build_cmake

    CMAKE=$INSTALLATION_PREFIX/bin/cmake
  fi
elif [ "$PREBUILD_ACTION" == "check" ]; then
  print_section "Checking build environment..."

  check_cmake
fi

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
