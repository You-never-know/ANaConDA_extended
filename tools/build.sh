#!/bin/bash
#
# Description:
#   A script simplifying building ANaConDA.
# Author:
#   Jan Fiedor
# Version:
#   0.5.2
# Created:
#   18.10.2013
# Last Update:
#   29.10.2013
#

source messages.sh

# Settings section
# ----------------

# GCC information
GCC_STABLE_VERSION=4.8.1
GCC_STABLE_DIR="gcc-$GCC_STABLE_VERSION"
GCC_STABLE_TGZ="$GCC_STABLE_DIR.tar.bz2"
GCC_STABLE_URL="http://ftp.fi.muni.cz/pub/gnu/gnu/gcc/$GCC_STABLE_DIR/$GCC_STABLE_TGZ"

# CMake information
CMAKE_STABLE_VERSION=2.8.12
CMAKE_STABLE_DIR="cmake-$CMAKE_STABLE_VERSION"
CMAKE_STABLE_TGZ="$CMAKE_STABLE_DIR.tar.gz"
CMAKE_STABLE_URL="http://www.cmake.org/files/v${CMAKE_STABLE_VERSION:0:3}/$CMAKE_STABLE_TGZ"

# Boost information
BOOST_STABLE_VERSION=1.54.0
BOOST_STABLE_DIR="boost_${BOOST_STABLE_VERSION//./_}"
BOOST_STABLE_TGZ="$BOOST_STABLE_DIR.tar.bz2"
BOOST_STABLE_URL="http://sourceforge.net/projects/boost/files/boost/$BOOST_STABLE_VERSION/$BOOST_STABLE_TGZ"

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
  $0 [--help] [--build-type { release | debug }] [--build-dir] [--install-dir]
     [--check-environment] [--setup-environment] [<target>]

Positional arguments:
  <target>  A target to build. Might be a required library, the framework itself
            or a specific analyser. Default is to build the ANaConDA framework.

optional arguments:
  --help
    Print the script usage.
  --build-type { release | debug }
    Build the release or debug version of the target, respectively. Default is
    to build the release version.
  --build-dir
    A path to a directory in which should the target be build. Default is the
    current working directory.
  --install-dir
    A path to a directory to which should the target be installed. Default is
    the current working directory.
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
#   Checks if a list contains a specific item.
# Parameters:
#   [STRING] A list.
#   [STRING] An item which the list should contain.
#   [STRING] A separator used to separate items in the list. If no separator is
#            specified, white spaces will be treated as separators.
# Output:
#   None
# Return:
#   0 if the list contains the specified item, 1 otherwise.
#
list_contains()
{
  # Helper variables
  local list=$1
  local searched_item=$2

  # If a separator is specified, override the global one
  if [ ! -z "$3" ]; then
    local IFS="$3"
  fi

  # Search for the item specified
  for item in $list; do
    if [ "$item" == "$searched_item" ]; then
      return 0 # Item found
    fi
  done

  return 1 # Item not found
}

#
# Description:
#   Initializes environment.
# Parameters:
#   None
# Output:
#   None
# Return:
#   Nothing
#
init_env()
{
  # Load the environment variables
  source ~/.anaconda/environment
}

#
# Description:
#   Escapes characters treated as special characters by the sed command.
# Parameters:
#   [STRING] A string.
# Output:
#   A string with escaped special characters.
# Return:
#   Nothing
#
sed_escape_special_chars()
{
  echo "$1" | sed 's!\([]\*\$\/&[]\)!\\\1!g'
}

#
# Description:
#   Updates an environment variable.
# Parameters:
#   [STRING] A name of the environment variable.
#   [STRING] A value of the environment variable.
# Output:
#   None
# Return:
#   Nothing
#
update_env_var()
{
  # Helper variables
  local environment_file=~/.anaconda/environment

  if [ ! -f "$environment_file" ]; then
    mkdir -p `dirname $environment_file`
    echo -n > $environment_file
  fi

  # Escape the name and value so we can use them with the sed command
  local sed_escaped_name="$(sed_escape_special_chars "$1")"
  local sed_escaped_value="$(sed_escape_special_chars "$2")"

  # Update the variable in the environment file first
  cat $environment_file | grep -E "^$1=" >/dev/null && sed -i -e "s/^$sed_escaped_name=.*$/$sed_escaped_name=$sed_escaped_value/" $environment_file || echo "$1=$2" >> $environment_file
}

#
# Description:
#   Checks if a supplied version meets the version requirements specified.
# Parameters:
#   [STRING] A minimum required version.
#   [STRING] A version to be checked.
# Output:
#   None
# Return:
#   0 if the specified version meets the version requirements, 1 otherwise.
#
check_version()
{
  # Helper variables
  local required_version=$1
  local supplied_version=$2

  if [ "$supplied_version" != "" ]; then
    # Some version supplied, check it against the required version
    local required_version_parts=( ${required_version//./ } 0 0 0 0 )
    local supplied_version_parts=( ${supplied_version//./ } 0 0 0 0 )

    for i in 0 1 2 3; do
      if [ ${supplied_version_parts[$i]} -lt ${required_version_parts[$i]} ]; then
        # The supplied version is older than the required version 
        return 1
      elif [ ${supplied_version_parts[$i]} -gt ${required_version_parts[$i]} ]; then
        # The supplied version is newer than the required version
        return 0
      fi
    done

    # The supplied version is the same as the version required
    return 0
  else
    # No version supplied
    return 1
  fi
}

#
# Description:
#   Checks if there exist a GCC compiler that meets the version requirements.
# Parameters:
#   [STRING] A name of the variable to which the path to the GCC compiler which
#            meets the version requirements will be stored.
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable GCC compiler was found, 1 otherwise.
#
check_gcc()
{
  # Helper variables
  local index

  # List of GCC compilers to check together with their description
  local gcc_compilers=("$CC" "$CXX" "g++" "$INSTALL_DIR/bin/g++")
  local gcc_compilers_desc=("\$CC variable" "\$CXX variable" "default g++" "local installation")

  print_subsection "checking GCC compiler"

  # Try to find a version of GCC which we can use the build the ANaConDA
  for index in ${!gcc_compilers[@]}; do
    print_info "     checking ${gcc_compilers_desc[$index]}... " -n

    local gcc_version=`${gcc_compilers[$index]} -v 2>&1 | grep -o -E "gcc version [0-9.]+" | grep -o -E "[0-9.]+"`

    if [ ! -z "$gcc_version" ]; then
      if check_version "4.7.0" $gcc_version; then
        print_info "success, version $gcc_version"

        if [ ! -z "$1" ]; then
          eval $1="'${gcc_compilers[$index]}'"
        fi

        return 0
      else
        print_info "fail, version $gcc_version"
      fi
    else
      print_info "fail, no version found"
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds GCC from its sources in the current directory.
# Parameters:
#   [STRING] A name of the variable to which the path to the GCC compiler which
#            was build will be stored.
# Output:
#   Detailed information about the building process.
# Return:
#   Nothing
#
build_gcc()
{
  print_subsection "building GCC compiler"

  # Download the archive containing the cmake source code
  print_info "     downloading... $GCC_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$GCC_STABLE_URL}

  # Extract the source code
  print_info "     extracting... $GCC_STABLE_TGZ"
  tar xvf ./$GCC_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $GCC_STABLE_DIR"
  cd $GCC_STABLE_DIR
  ./configure --enable-languages=c++,c --disable-bootstrap --disable-multilib --prefix=$INSTALL_DIR --with-gmp=/usr --with-mpc=/usr --with-mpfr=/usr && make && make -j1 install || terminate "cannot build GCC."
  cd ..

  # Save the path to the compiled GCC compiler if requested
  if [ ! -z "$1" ]; then
    eval $1="'$INSTALL_DIR/bin/g++'"
  fi
}

#
# Description:
#   Checks if there exist a CMake binary that meets the version requirements.
# Parameters:
#   [STRING] A name of the variable to which the path to the CMake binary which
#            meets the version requirements will be stored.
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable CMake binary was found, 1 otherwise.
#
check_cmake()
{
  # Helper variables
  local index

  # List of CMake binaries to check together with their description
  local cmake_binaries=("$CMAKE" "cmake" "$INSTALL_DIR/bin/cmake")
  local cmake_binaries_desc=("\$CMAKE variable" "default cmake" "local installation")

  print_subsection "checking CMake build system"

  # Try to find a version of CMake which we can use the build the ANaConDA
  for index in ${!cmake_binaries[@]}; do
    print_info "     checking ${cmake_binaries_desc[$index]}... " -n

    local cmake_version=`${cmake_binaries[$index]} --version 2>&1 | grep -o -E "cmake version [0-9.]+" | grep -o -E "[0-9.]+"`

    if [ ! -z "$cmake_version" ]; then
      if check_version "2.8.3" $cmake_version; then
        print_info "success, version $cmake_version"

        if [ ! -z "$1" ]; then
          eval $1="'${cmake_binaries[$index]}'"
        fi

        return 0
     else
       print_info "fail, version $cmake_version"
      fi
    else
      print_info "fail, no version found"
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds CMake from its sources in the current directory.
# Parameters:
#   [STRING] A name of the variable to which the path to the CMake binary which
#            was build will be stored.
# Output:
#   Detailed information about the building process.
# Return:
#   Nothing
#
build_cmake()
{
  print_subsection "building CMake"

  # Download the archive containing the cmake source code
  print_info "     downloading... $CMAKE_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$CMAKE_STABLE_URL}

  # Extract the source code
  print_info "     extracting... $CMAKE_STABLE_TGZ"
  tar xvf ./$CMAKE_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $CMAKE_STABLE_DIR"
  cd $CMAKE_STABLE_DIR
  ./bootstrap --prefix=$INSTALL_DIR && make && make install || terminate "cannot build CMake."
  cd ..

  # Save the path to the compiled CMake binary if requested
  if [ ! -z "$1" ]; then
    eval $1="'$INSTALL_DIR/bin/cmake'"
  fi
}

#
# Description:
#   Checks if there exist Boost libraries that meets the version requirements.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable Boost libraries were found, 1 otherwise.
#
check_boost()
{
  # Helper variables
  local check_boost_temp_dir="./boost-check"

  print_subsection "checking Boost libraries"

  # Prepare a temporary directory for storing files produced during the check
  mkdir -p $check_boost_temp_dir && cd $check_boost_temp_dir

  # Prepare a CMake script which checks the version of Boost libraries
  echo -e "
    cmake_minimum_required(VERSION 2.8.3)
    set(Boost_USE_MULTITHREADED FALSE)
    find_package(Boost 1.46.0 COMPONENTS date_time filesystem program_options regex system)
    if (Boost_FOUND)
      message(\"Boost_INCLUDE_DIRS=\${Boost_INCLUDE_DIRS}\")
    endif (Boost_FOUND)
  " > CMakeLists.txt

  # Use CMake to check the version of Boost libraries
  local boost_info=`$CMAKE . CMakeLists.txt 2>&1`

  # Clean everything up
  cd .. && rm -rf $check_boost_temp_dir

  # Check the version of boost
  print_info "     checking boost version... " -n

  local boost_version=`echo "$boost_info" | grep -E "^-- Boost version: [0-9.]+$" | grep -E -o "[0-9.]+"`

  if [ ! -z "$boost_version" ]; then
    if check_version "1.46.0" $boost_version; then
      print_info "success, version $boost_version"

      return 0
    else
      print_info "fail, version $boost_version"
    fi
  else
    print_info "fail, no version found"
  fi

  return 1
}

#
# Description:
#   Builds Boost from its sources in the current directory.
# Parameters:
#   [STRING] A name of the variable to which the path to the Boost library which
#            was build will be stored.
# Output:
#   Detailed information about the building process.
# Return:
#   Nothing
#
build_boost()
{
  print_subsection "building Boost"

  # Download the archive containing the cmake source code
  print_info "     downloading... $BOOST_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$BOOST_STABLE_URL}

  # Extract the source code
  print_info "     extracting... $BOOST_STABLE_TGZ"
  tar xvf ./$BOOST_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $BOOST_STABLE_DIR"
  cd $BOOST_STABLE_DIR
  ./bootstrap.sh --prefix=$INSTALL_DIR --with-libraries=date_time,filesystem,program_options,regex,system && ./b2 install || terminate "cannot build Boost."
  cd ..

  # Save the path to the compiled CMake binary if requested
  if [ ! -z "$1" ]; then
    eval $1="'$INSTALL_DIR'"
  fi
}

# Program section
# ---------------

# Save information about the directory in which we executed the script
SCRIPT_DIR=`pwd`

# Default values for optional parameters
BUILD_TYPE=release
BUILD_DIR=$SCRIPT_DIR
INSTALL_DIR=$SCRIPT_DIR
PREBUILD_ACTION=none

# Initialize environment first, optional parameters might override the values
init_env

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
      BUILD_TYPE=$2
      shift
      ;;
    "--build-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the build directory."
      fi
      BUILD_DIR=$2
      shift
      ;;
    "--install-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the installation directory."
      fi
      INSTALL_DIR=$2
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

# Process the positional parameters
if [ -z "$1" ]; then
  BUILD_TARGET=anaconda
else
  BUILD_TARGET=$1
fi

print_section "Preparing build script..."

print_subsection "checking build settings"

print_info "     build type... " -n

if [[ "$BUILD_TYPE" =~ ^release|debug$ ]]; then
  print_info "$BUILD_TYPE"
else
  print_info "invalid"
  terminate "build type must be release or debug."
fi

print_info "     build directory... " -n

if [ -d "$BUILD_DIR" ]; then
  print_info "$BUILD_DIR"
else
  if mkdir -p "$BUILD_DIR"; then
    print_info "$BUILD_DIR"
  else
    print_info "not found"
    terminate "directory $BUILD_DIR cannot be created."
  fi
fi

print_info "     installation directory... " -n

if [ -d "$INSTALL_DIR" ]; then
  print_info "$INSTALL_DIR"
else
  if mkdir -p "$INSTALL_DIR"; then
    print_info "$INSTALL_DIR"
  else
    print_info "not found"
    terminate "directory $INSTALL_DIR cannot be created."
  fi
fi

print_subsection "configuring build script"

print_info "     download command... " -n

# Setup the command used to download files from the internet or local network
if [ -z "$DOWNLOAD_COMMAND" ]; then
  # If no download command is set, check if wget is available
  WGET_VERSION=`wget -V 2>&1 | grep -o -E "Wget.[0-9.]+"`

  if [ "$WGET_VERSION" != "" ]; then
    DOWNLOAD_COMMAND="wget --passive-ftp -c %u"
  fi
fi

print_info "$DOWNLOAD_COMMAND"

cd $BUILD_DIR

# Execute all requested prebuild actions
if [ "$PREBUILD_ACTION" == "setup" ]; then
  print_section "Setting up build environment..."

  if ! check_gcc CC; then
    build_gcc CC

    update_env_var CC $CC
  fi

  if ! check_cmake CMAKE; then
    build_cmake CMAKE

    update_env_var CMAKE $CMAKE
  fi

  if ! check_boost; then
    build_boost
  fi
elif [ "$PREBUILD_ACTION" == "check" ]; then
  print_section "Checking build environment..."

  check_gcc
  check_cmake
  check_boost
fi

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
