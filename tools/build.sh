#!/bin/bash

#
# Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
#   A script simplifying building of the ANaConDA framework and its parts.
# Author:
#   Jan Fiedor
# Version:
#   3.2.8
# Created:
#   18.10.2013
# Last Update:
#   27.01.2020
#

# Search the folder containing the script for the included scripts
PATH=$PATH:$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Include required scripts
source utils.sh

# Settings section
# ----------------

# GCC information
GCC_STABLE_VERSION=4.9.4
GCC_STABLE_DIR="gcc-$GCC_STABLE_VERSION"
GCC_STABLE_TGZ="$GCC_STABLE_DIR.tar.bz2"
GCC_STABLE_URL="http://ftp.fi.muni.cz/pub/gnu/gnu/gcc/$GCC_STABLE_DIR/$GCC_STABLE_TGZ"
GCC_STABLE_MIRRORS="mirrors.html"
GCC_STABLE_MIRRORS_URL="http://gcc.gnu.org/$GCC_STABLE_MIRRORS"
GCC_STABLE_MIRRORS_TRIES=2
GCC_STABLE_MIRRORS_TIMEOUT=40

# CMake information
CMAKE_STABLE_VERSION=3.3.0
CMAKE_STABLE_DIR="cmake-$CMAKE_STABLE_VERSION"
CMAKE_STABLE_TGZ="$CMAKE_STABLE_DIR.tar.gz"
CMAKE_STABLE_URL="http://www.cmake.org/files/v${CMAKE_STABLE_VERSION:0:3}/$CMAKE_STABLE_TGZ"
CMAKE_STABLE_INSTALLER="$CMAKE_STABLE_DIR-win32-x86.exe"
CMAKE_STABLE_INSTALLER_URL="http://www.cmake.org/files/v${CMAKE_STABLE_VERSION:0:3}/$CMAKE_STABLE_INSTALLER"

# Boost information
BOOST_STABLE_VERSION=1.58.0
BOOST_STABLE_DIR="boost_${BOOST_STABLE_VERSION//./_}"
BOOST_STABLE_TGZ="$BOOST_STABLE_DIR.tar.bz2"
BOOST_STABLE_URL="http://sourceforge.net/projects/boost/files/boost/$BOOST_STABLE_VERSION/$BOOST_STABLE_TGZ"
BOOST_STABLE_INSTALLER_32="boost_${BOOST_STABLE_VERSION//./_}-msvc-12.0-32.exe"
BOOST_STABLE_INSTALLER_64="boost_${BOOST_STABLE_VERSION//./_}-msvc-12.0-64.exe"
BOOST_STABLE_INSTALLER_URL_32="http://sourceforge.net/projects/boost/files/boost-binaries/$BOOST_STABLE_VERSION/$BOOST_STABLE_INSTALLER_32"
BOOST_STABLE_INSTALLER_URL_64="http://sourceforge.net/projects/boost/files/boost-binaries/$BOOST_STABLE_VERSION/$BOOST_STABLE_INSTALLER_64"

# PIN platform-dependent information
if [ "$HOST_OS" == "windows" ]; then
  PIN_STABLE_OS="windows"
  PIN_STABLE_COMPILER="msvc12"
  PIN_STABLE_ARCHIVE_EXT=".zip"
elif [ "$HOST_OS" == "linux" ]; then
  PIN_STABLE_OS="linux"
  PIN_STABLE_COMPILER="gcc.4.4.7"
  PIN_STABLE_ARCHIVE_EXT=".tar.gz"
elif [ "$HOST_OS" == "mac" ]; then
  PIN_STABLE_OS="mac"
  PIN_STABLE_COMPILER="clang.4.2"
  PIN_STABLE_ARCHIVE_EXT=".tar.gz"
fi

# PIN information
PIN_STABLE_VERSION=2.14
PIN_STABLE_REVISION=71313
PIN_STABLE_DIR="pin-$PIN_STABLE_VERSION-$PIN_STABLE_REVISION-$PIN_STABLE_COMPILER-$PIN_STABLE_OS"
PIN_STABLE_ARCHIVE="$PIN_STABLE_DIR$PIN_STABLE_ARCHIVE_EXT"
PIN_STABLE_ARCHIVE_URL="http://software.intel.com/sites/landingpage/pintool/downloads/$PIN_STABLE_ARCHIVE"

# Libdwarf information
LIBDWARF_STABLE_VERSION=20150507
LIBDWARF_STABLE_DIR="dwarf-$LIBDWARF_STABLE_VERSION"
LIBDWARF_STABLE_TGZ="lib$LIBDWARF_STABLE_DIR.tar.gz"
LIBDWARF_STABLE_URL="http://www.prevanders.net/$LIBDWARF_STABLE_TGZ"

# Libelf information
LIBELF_STABLE_VERSION=0.157
LIBELF_STABLE_DIR="elfutils-$LIBELF_STABLE_VERSION"
LIBELF_STABLE_TGZ="$LIBELF_STABLE_DIR.tar.bz2"
LIBELF_STABLE_URL="http://fedorahosted.org/releases/e/l/elfutils/$LIBELF_STABLE_VERSION/$LIBELF_STABLE_TGZ"

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
  $0 [--help] [--clean] [--build-type { release | debug }]
     [--build-dir] [--install-dir] [--source-dir]
     [--check-environment] [--setup-environment]
     [--check-runtime] [--setup-runtime]
     [--verbose] [--target-arch { x86_64 | x86 }]
     [--test]
     [<target>]

positional arguments:
  <target>  A target to build. Might be a required library, the framework itself
            or a specific analyser. Default is to build the ANaConDA framework.
            If --check-environment or --setup-environment is used, the target
            might be omitted.

optional arguments:
  --help
    Print the script usage.
  --clean
    Perform a clean build, i.e., clean the target before building it. Note that
    this cleanup is performed in the build directory, where the target's source
    files are copied during the build process, and not in the source directory,
    which might be a different directory. If no target is specified, clean all
    targets in the build directory.
  --build-type { release | debug }
    Build the release or debug version of the target, respectively. Default is
    to build the release version.
  --build-dir
    A path to a directory in which should the target be build. Default is the
    current working directory.
  --install-dir
    A path to a directory to which should the target be installed. Default is
    the current working directory.
  --source-dir
    A path to a directory contaning source files needed to build the targets.
    Default is the current working directory.
  --check-environment
    Check if the tools necessary for building ANaConDA are available.
  --setup-environment
    Setup the environment to be able to build ANaConDA (e.g. installs all the
    tools needed to build ANaConDA which are not available).
  --check-runtime
    Check if the tools necessary for running ANaConDA are available.
  --setup-runtime
    Setup the environment to be able to run ANaConDA (e.g. installs all the
    tools needed to run ANaConDA which are not available).
  --verbose
    Show detailed information about the build process, e.g., commands used to
    compile the target, etc.
  --target-arch { x86_64 | x86 }
    Build the 64-bit or 32-bit version of the target, respectively. Default is
    the version matching the version of the operating system, i.e., 64-bit for
    a 64-bit operating system and 32-bit for a 32-bit operating system.
  --test
    Test the target after building it. Note that some tests may require other
    targets to be build first in order to work properly.
"
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
#   None
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
  local gcc_compilers=("$GCC_HOME/bin/g++" "$INSTALL_DIR/bin/g++" "g++" "$CC" "$CXX")
  local gcc_compilers_desc=("preferred installation" "local installation" "default g++" "CC variable" "CXX variable")

  print_subsection "checking GCC compiler"

  # Try to find a version of GCC which we can use the build the ANaConDA
  for index in ${!gcc_compilers[@]}; do
    print_info "     checking ${gcc_compilers_desc[$index]}... " -n

    local gcc_version=`${gcc_compilers[$index]} -v 2>&1 | grep -o -E "gcc version [0-9.]+" | grep -o -E "[0-9.]+"`

    if [ ! -z "$gcc_version" ]; then
      if check_version "4.9.3" $gcc_version; then
        if check_version "5.1.0" $gcc_version; then
          # GCC from version 5.1 has dual ABI, which is causing problems
          local is_gcc4_compatible=`${gcc_compilers[$index]} -v 2>&1 | grep "\-\-with-default\-libstdcxx\-abi=gcc4\-compatible"`

          if [ -z "$is_gcc4_compatible" ]; then
            # GCC was not compiled with the old ABI as default
            print_info "fail, version $gcc_version (incompatible ABI)"

            continue
          fi
        fi

        print_info "success, version $gcc_version"

        env_update_var GCC_HOME "$(dirname $(which ${gcc_compilers[$index]}) | sed -e 's/^\(.*\)\/bin$/\1/')"

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
#   Downloads the prerequisites needed to build GCC.
# Parameters:
#   None
# Output:
#   None
# Return:
#   0 if all prerequisites were downloaded successfully, 1 otherwise.
#
download_gcc_prerequisites()
{
  # No prerequisites were downloaded yet
  local result=1

  if [ ! -f ./contrib/download_prerequisites.orig ]; then
    # Backup the original file, we will be modifying it several times
    cp ./contrib/download_prerequisites ./contrib/download_prerequisites.orig
  fi

  # Get a list of (mirror) sites containing the prerequisites
  ${DOWNLOAD_COMMAND//%u/$GCC_STABLE_MIRRORS_URL} || terminate "cannot download the list of sites containing the GCC prerequisites."

  for mirror in `cat $GCC_STABLE_MIRRORS | awk 'BEGIN {print "href=\"ftp://gcc.gnu.org/pub/gcc\""} /GCC mirror sites/ {m=1} /<ul>/ {if (m==1) {p=1}} /<\/ul>/ {m=0; p=0} p==1 {print $0}' | grep href | sed -e 's/.*href="\([^"]*\)".*/\1/' | sed -e 's/^\(.*\)\/$/\1/'`; do
    # Try to download the prerequsities from one of the mirror sites
    # (the main site is explicitly added as the first mirror site)
    cp -f ./contrib/download_prerequisites.orig ./contrib/download_prerequisites
    # Escape the mirror site url so we can use it with the sed command
    local sed_escaped_mirror="$(sed_escape_special_chars "$mirror")"
    # Replace the main site with the currently checked mirror site
    sed -i -e "s/wget ftp:\/\/gcc.gnu.org\/pub\/gcc/wget -c --tries=$GCC_STABLE_MIRRORS_TRIES --timeout=$GCC_STABLE_MIRRORS_TIMEOUT $sed_escaped_mirror/g" ./contrib/download_prerequisites

    if ./contrib/download_prerequisites; then
      # Successfully downloaded the prerequisites
      result=0
      break
    fi
  done

  # Restore original and clean temporary files
  cp -f ./contrib/download_prerequisites.orig ./contrib/download_prerequisites
  rm -rf $GCC_STABLE_MIRRORS ./contrib/download_prerequisites.orig

  # If we successfully downloaded the prerequisites, we return 0
  return $result
}

#
# Description:
#   Downloads and applies patches needed to build GCC from its sources.
# Parameters:
#   None
# Output:
#   Detailed information about the patch process.
# Return:
#   Nothing
#
patch_gcc()
{
  # Use patches from the buildroot project backported to GCC 4.9.4
  local buildroot_gcc_494_patches_rooturl="https://raw.githubusercontent.com/rancher/buildroot/master/package/gcc/4.9.4/"

  # A list of patches to apply
  local gcc_patches=()

  print_info "     building a list of required patches..."

  # Determine the version of the glibc library
  local glibc_version=`ldd --version 2>&1 | sed -n -E "s/ldd \([^()]+\) ([0-9.]+)/\1/p"`

  if check_version "2.25.0" $glibc_version; then
    # The following patches fix compilation errors when compiling the GCC source
    # code with glibc 2.25 (or newer)
    gcc_patches+=("942-asan-fix-missing-include-signal-h.patch")
  fi

  if check_version "2.26.0" $glibc_version; then
    # The following patches fix compilation errors when compiling the GCC source
    # code with glibc 2.26 (or newer)
    gcc_patches+=(
      "943-Use-ucontext_t-not-struct-ucontext-in-linux-unwind.h.patch"
      "944-sanitizer-linux.patch")
  fi

  if check_version "2.28.0" $glibc_version; then
    # The following patches fix compilation errors when compiling the GCC source
    # code with glibc 2.28 (or newer)
    gcc_patches+=("0003-libsanitizer-Use-pre-computed-size.patch")
  fi

  # Print the list of required patches
  for index in ${!gcc_patches[@]}; do
    print_info "${gcc_patches[$index]}"
  done

  # Download the patches to the directory where the GCC source code is
  cd $GCC_STABLE_DIR

  for index in ${!gcc_patches[@]}; do
    # Download the file containing the patch
    print_info "     downloading patch... ${gcc_patches[$index]}"
    ${DOWNLOAD_COMMAND//%u/$buildroot_gcc_494_patches_rooturl${gcc_patches[$index]}} || terminate "cannot download patch ${gcc_patches[$index]}."

    # Apply the patch on the source code
    print_info "     applying patch... ${gcc_patches[$index]}"
    patch -p1 < "./${gcc_patches[$index]}"
  done
}

#
# Description:
#   Builds GCC from its sources in the current directory.
# Parameters:
#   None
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_gcc()
{
  print_subsection "building GCC compiler"

  # Check if any C compiler is present to build gcc
  print_info "     checking current C compiler... " -n

  local gcc_version=`gcc -v 2>&1 | grep -o -E "gcc version [0-9.]+" | grep -o -E "[0-9.]+"`

  if [ ! -z "$gcc_version" ]; then
    print_info "gcc $gcc_version"
  else
    print_info "not found"

    terminate "no C compiler found."
  fi

  # Check if any C++ compiler is present to build g++
  print_info "     checking current C++ compiler... " -n

  local gpp_version=`g++ -v 2>&1 | grep -o -E "gcc version [0-9.]+" | grep -o -E "[0-9.]+"`

  if [ ! -z "$gpp_version" ]; then
    print_info "g++ $gpp_version"
  else
    print_info "not found"

    terminate "no C++ compiler found."
  fi

  # Download the archive containing the GCC source code
  print_info "     downloading... $GCC_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$GCC_STABLE_URL} || terminate "cannot download GCC source archive."

  # Extract the source code
  print_info "     extracting... $GCC_STABLE_TGZ"
  tar xf ./$GCC_STABLE_TGZ

  # Patch the source code
  print_info "     patching... $GCC_STABLE_DIR"
  patch_gcc

  # Compile the source code
  print_info "     compiling... $GCC_STABLE_DIR"
  cd $GCC_STABLE_DIR
  if ! download_gcc_prerequisites; then
    terminate "cannot download libraries needed to build GCC."
  fi
  cd ..
  mkdir -p gcc-build
  cd gcc-build
  ../$GCC_STABLE_DIR/configure --enable-languages=c++,c --disable-bootstrap --disable-multilib --prefix=$INSTALL_DIR && make && make -j1 install || terminate "cannot build GCC."
  cd ..

  # Update the environment
  env_update_var GCC_HOME "$INSTALL_DIR"
}

#
# Description:
#   Checks if there exist a CMake binary that meets the version requirements.
# Parameters:
#   None
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
  local cmake_binaries_desc=("CMAKE variable" "default cmake" "local installation")

  # On Windows, search also the registry for CMake binaries
  if [ "$HOST_OS" == "windows" ]; then
    # Recursive search in the 32-bit portion of the registry, path in (Default)
    local cmake_install_dir=`reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Kitware" /reg:32 /s 2>&1 | grep Default | sed "s/^.*REG_SZ[ ]*\(.*\)/\1/"`

    if [ ! -z "$cmake_install_dir" ]; then
      # The path in registry is Windows-style and points to the install dir
      cmake_binaries+=("`cygpath -u $cmake_install_dir`/bin/cmake")
      cmake_binaries_desc+=("registry")
    fi
  fi

  print_subsection "checking CMake build system"

  # Try to find a version of CMake which we can use to build the ANaConDA
  for index in ${!cmake_binaries[@]}; do
    print_info "     checking ${cmake_binaries_desc[$index]}... " -n

    local cmake_version=`${cmake_binaries[$index]} --version 2>&1 | grep -o -E "cmake version [0-9.]+" | grep -o -E "[0-9.]+"`

    if [ ! -z "$cmake_version" ]; then
      if check_version "3.2" $cmake_version; then
        if [ "$HOST_OS" == "windows" ]; then
          # We are running in Cygwin, however, we cannot use its CMake
          local check_cmake_temp_dir="./cmake-check"

          # Prepare a directory for storing files produced during the check
          mkdir -p $check_cmake_temp_dir && cd $check_cmake_temp_dir

          # Prepare a CMake script which checks the platform of the CMake found
          echo -e "
            cmake_minimum_required(VERSION 2.8.3)
            project(cmake-check NONE)
            message(\"CYGWIN=\${CYGWIN}\")
          " > CMakeLists.txt

          # Use CMake to check if the CMake found is from Cygwin or Windows
          local cmake_info=`${cmake_binaries[$index]} . CMakeLists.txt 2>&1`

          # Clean everything up
          cd .. && rm -rf $check_cmake_temp_dir

          # Determine if the version found is a version for Cygwin or not
          local cmake_platform_info=`echo "$cmake_info" | grep CYGWIN=1`

          if [ -z "$cmake_platform_info" ]; then
            # Version for Windows, we can use this version
            print_info "success, version $cmake_version"

            env_update_var CMAKE "${cmake_binaries[$index]}"

            return 0
          else
            # Version for Cygwin, cannot be used to build ANaConDA
            print_info "fail, version not for Windows"
          fi
        else
          print_info "success, version $cmake_version"

          env_update_var CMAKE "${cmake_binaries[$index]}"

          return 0
        fi
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
#   None
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_cmake()
{
  print_subsection "building CMake"

  # Download the archive containing the CMake source code
  print_info "     downloading... $CMAKE_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$CMAKE_STABLE_URL} || terminate "cannot download CMake source archive."

  # Extract the source code
  print_info "     extracting... $CMAKE_STABLE_TGZ"
  tar xf ./$CMAKE_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $CMAKE_STABLE_DIR"
  cd $CMAKE_STABLE_DIR
  ./bootstrap --prefix=$INSTALL_DIR && make && make install || terminate "cannot build CMake."
  cd ..

  # Update the environment
  env_update_var CMAKE "$INSTALL_DIR/bin/cmake"
}

#
# Description:
#   Installs CMake.
# Parameters:
#   None
# Output:
#   Detailed information about the installation process.
# Return:
#   Nothing
#
install_cmake()
{
  print_subsection "installing CMake"

  # Download the CMake Windows installer
  print_info "     downloading... $CMAKE_STABLE_INSTALLER_URL"
  ${DOWNLOAD_COMMAND//%u/$CMAKE_STABLE_INSTALLER_URL} || terminate "cannot download CMake installer."

  # Install the CMake to the target directory
  print_info "     installing... $CMAKE_STABLE_INSTALLER"
  # The installation directory must be a Windows path
  local cmake_install_dir="$INSTALL_DIR/CMake"
  correct_paths cmake_install_dir
  # Silent installation (/S) to the installation directory (/D)
  cygstart --action=runas --wait ./$CMAKE_STABLE_INSTALLER /S /D=${cmake_install_dir//\//\\}

  # Update the environment
  env_update_var CMAKE "$INSTALL_DIR/CMake/bin/cmake"
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
  local index

  # List of Boost paths to check together with their description
  local boost_paths=("$BOOST_ROOT" "$BOOST_HOME" "$INSTALL_DIR")
  local boost_paths_desc=("BOOST_ROOT variable" "BOOST_HOME variable" "local installation")

  # Search also the subfolders of the installation directory for local installations
  for boost_path in `find $INSTALL_DIR -mindepth 1 -maxdepth 1 -type d -iname "boost*"`; do
    boost_paths+=("$boost_path")
    boost_paths_desc+=("local installation ($boost_path)")
  done

  print_subsection "checking Boost libraries"

  # Check if CMake is available, without it, we cannot perform the checks
  if [ -z "$CMAKE" ]; then
    print_info "     checking failed, CMake not found"

    return 1
  fi

  # Try to find a version of Boost which we can use to build the ANaConDA
  for index in ${!boost_paths[@]}; do
    print_info "     checking ${boost_paths_desc[$index]}... " -n

    # Prepare a temporary directory for storing files produced during the check
    mkdir -p $check_boost_temp_dir && cd $check_boost_temp_dir

    # Path to a directory containing CMake helper scripts
    local module_path="$SOURCE_DIR/shared/cmake"
    # CMake for Windows requires this path to be in Windows format
    if [ "$HOST_OS" == "windows" ]; then
      correct_paths module_path
    fi

    # Prepare a CMake script which checks the version of Boost libraries
    echo -e "
      cmake_minimum_required(VERSION 2.8.3)
      project(boost-check CXX)
      add_library(boost-check SHARED)
      set(CMAKE_MODULE_PATH \"$module_path\")
      include(SetupBoost)
      SETUP_BOOST(boost-check 1.46.0 date_time filesystem program_options system)
      if (Boost_FOUND)
        message(\"Boost_INCLUDE_DIRS=\${Boost_INCLUDE_DIRS}\")
      endif (Boost_FOUND)
    " > CMakeLists.txt

    # On Windows, use the Unix makefiles generator or else Visual Studio 2013
    # generator will be used which uses the 32-bit version of the compiler by
    # default so we will always check for 32-bit version of Boost with it :S
    if [ "$HOST_OS" == "windows" ]; then
      local cmake_flags=("-DCMAKE_CXX_COMPILER=cl" "-GUnix Makefiles")
    fi

    # Use CMake to check the version of Boost libraries
    local boost_info=`BOOST_ROOT="${boost_paths[$index]}" $CMAKE "${cmake_flags[@]}" . CMakeLists.txt 2>&1`

    # Clean everything up
    cd .. && rm -rf $check_boost_temp_dir

    # If Boost is found, get its version 
    local boost_version=`echo "$boost_info" | grep -E "^-- Boost version: [0-9.]+$" | grep -E -o "[0-9.]+"`

    if [ ! -z "$boost_version" ]; then
      if check_version "1.46.0" $boost_version; then
        print_info "success, version $boost_version"

        local boost_include_dirs=`echo "$boost_info" | grep -o -E "^Boost_INCLUDE_DIRS=.*$" | sed -e "s/^Boost_INCLUDE_DIRS=\(.*\)$/\1/"`

        if [ "$HOST_OS" == "windows" ]; then
          local boost_root_dir="$(echo "$boost_include_dirs" | sed -e 's/^\(.*\)\/include$/\1/' | cygpath -u -f -)"
        else
          local boost_root_dir="$(echo "$boost_include_dirs" | sed -e 's/^\(.*\)\/include$/\1/')"
        fi

        env_update_var BOOST_ROOT "$boost_root_dir"

        return 0
      else
        print_info "fail, version $boost_version"
      fi
    else
      print_info "fail, no version found"

      # Print detailed information about why the Boost could not be found
      if [ "$VERBOSE" == "1" ]; then
        echo -e "\n$boost_info\n"
      fi
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds Boost from its sources in the current directory.
# Parameters:
#   None
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_boost()
{
  print_subsection "building Boost"

  # Download the archive containing the Boost source code
  print_info "     downloading... $BOOST_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$BOOST_STABLE_URL} || terminate "cannot download Boost source archive."

  # Extract the source code
  print_info "     extracting... $BOOST_STABLE_TGZ"
  tar xf ./$BOOST_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $BOOST_STABLE_DIR"
  cd $BOOST_STABLE_DIR
  ./bootstrap.sh --prefix=$INSTALL_DIR --with-libraries=date_time,filesystem,program_options,system && ./b2 install || terminate "cannot build Boost."
  cd ..

  # Update the environment
  env_update_var BOOST_ROOT "$INSTALL_DIR"
}

#
# Description:
#   Installs Boost.
# Parameters:
#   None
# Output:
#   Detailed information about the installation process.
# Return:
#   Nothing
#
install_boost()
{
  print_subsection "installing Boost"

  # Determine which version of the Boost libraries we need to install
  if [ "$TARGET_ARCH" == "x86_64" ]; then
    BOOST_STABLE_INSTALLER=$BOOST_STABLE_INSTALLER_64
    BOOST_STABLE_INSTALLER_URL=$BOOST_STABLE_INSTALLER_URL_64
  else
    BOOST_STABLE_INSTALLER=$BOOST_STABLE_INSTALLER_32
    BOOST_STABLE_INSTALLER_URL=$BOOST_STABLE_INSTALLER_URL_32
  fi

  # Download the Boost Windows installer
  print_info "     downloading... $BOOST_STABLE_INSTALLER_URL"
  ${DOWNLOAD_COMMAND//%u/$BOOST_STABLE_INSTALLER_URL} || terminate "cannot download Boost installer."

  # Install the Boost to the target directory
  print_info "     installing... $BOOST_STABLE_INSTALLER"
  # The installation directory must be a Windows path
  local boost_install_dir="$INSTALL_DIR/Boost"
  correct_paths boost_install_dir
  # Silent installation (/SILENT) to the installation directory (/DIR)
  cygstart --action=runas --wait ./$BOOST_STABLE_INSTALLER /SILENT /DIR="${boost_install_dir//\//\\}"

  # Update the environment
  env_update_var BOOST_ROOT "$INSTALL_DIR/Boost"
}

#
# Description:
#   Checks if there exist PIN framework that meets the version requirements.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable PIN framework was found, 1 otherwise.
#
check_pin()
{
  # Helper variables
  local index

  # List of PIN binaries to check together with their description
  local pin_binaries=("$PIN_HOME/pin" "$INSTALL_DIR/opt/pin/pin" "pin")
  local pin_binaries_desc=("preferred installation" "local installation" "default PIN")

  # Search also the subfolders of the installation directory for local installations
  for pin_path in `find $INSTALL_DIR -mindepth 1 -maxdepth 1 -type d -iname "pin*"`; do
    pin_binaries+=("$pin_path/pin")
    pin_binaries_desc+=("local installation ($pin_path)")
  done

  print_subsection "checking PIN framework"

  # Even the latest version of PIN does not support Linux kernel 4.x yet
  if [ "$HOST_OS" == "linux" ]; then
    if [ `uname -r | sed "s/^\([0-9.]*\).*$/\1/" | cut -f1 -d.` -ge 4 ]; then
      # This undocumented switch will disable the kernel version check
      PIN_FLAGS=-ifeellucky
    fi
  fi

  # Try to find any version of PIN which we can use to run the ANaConDA
  for index in ${!pin_binaries[@]}; do
    print_info "     checking ${pin_binaries_desc[$index]}... " -n

    # Use the same launcher that the run.sh and test.sh scripts are using
    if [ "$HOST_OS" == "windows" ]; then
      local pin_launcher="${pin_binaries[$index]}.exe"
    else
      local pin_launcher="${pin_binaries[$index]}.sh"
    fi

    local pin_version=`$pin_launcher -version $PIN_FLAGS 2>&1 | grep -o -E "^Pin [0-9.]+" | grep -o -E "[0-9.]+"`

    if [ ! -z "$pin_version" ]; then
      if check_version "2.14" $pin_version; then
        print_info "success, version $pin_version"

        env_update_var PIN_HOME "$(dirname $(which ${pin_binaries[$index]}))"
        env_update_var LIBDWARF_ROOT "$PIN_HOME/$PIN_TARGET_LONG/lib-ext"
        env_update_var LIBELF_ROOT "$PIN_HOME/$PIN_TARGET_LONG/lib-ext"

        return 0
      else
        print_info "fail, version $pin_version"
      fi
    else
      print_info "fail, no version found"
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Installs PIN.
# Parameters:
#   None
# Output:
#   Detailed information about the installation process.
# Return:
#   Nothing
#
install_pin()
{
  print_subsection "installing PIN"

  # Download the archive containing the PIN framework
  print_info "     downloading... $PIN_STABLE_ARCHIVE_URL"
  ${DOWNLOAD_COMMAND//%u/$PIN_STABLE_ARCHIVE_URL} || terminate "cannot download PIN installation archive."

  # Extract the PIN framework to the target directory
  print_info "     extracting... $PIN_STABLE_ARCHIVE"

  if [ "$HOST_OS" == "windows" ]; then
    local pin_install_dir="$INSTALL_DIR"
  else
    local pin_install_dir="$INSTALL_DIR/opt"
  fi

  mkdir -p $pin_install_dir

  if [[ "$PIN_STABLE_ARCHIVE" =~ .*\.tar\.gz$ ]]; then
    if [ "$HOST_OS" == "mac" ]; then
      tar -C "$pin_install_dir" -xf ./$PIN_STABLE_ARCHIVE 2>&1 &> /dev/null
      mv "$pin_install_dir/$PIN_STABLE_DIR" "$pin_install_dir/pin"
    else
      tar --transform="s/^$PIN_STABLE_DIR/pin/" --directory="$pin_install_dir" -xf ./$PIN_STABLE_ARCHIVE 2>&1 &> /dev/null
    fi
  elif [[ "$PIN_STABLE_ARCHIVE" =~ .*\.zip$ ]]; then
    unzip $PIN_STABLE_ARCHIVE -d "$pin_install_dir" 2>&1 &> /dev/null
    mv "$pin_install_dir/$PIN_STABLE_DIR" "$pin_install_dir/Pin"
  else
    terminate "cannot extract $PIN_STABLE_ARCHIVE, unsupported format."
  fi

  # Update the environment
  env_update_var PIN_HOME "$pin_install_dir/pin"
  env_update_var LIBDWARF_ROOT "$PIN_HOME/$PIN_TARGET_LONG/lib-ext"
  env_update_var LIBELF_ROOT "$PIN_HOME/$PIN_TARGET_LONG/lib-ext"
}

#
# Description:
#   Checks if there exist libdwarf library that meets the version requirements.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable libdwarf library was found, 1 otherwise.
#
check_libdwarf()
{
  # Helper variables
  local check_libdwarf_temp_dir="./libdwarf-check"
  local index

  # List of libdwarf paths to check together with their description
  local libdwarf_paths=("$LIBDWARF_HOME" "$LIBDWARF_ROOT" "$INSTALL_DIR")
  local libdwarf_paths_desc=("LIBDWARF_HOME variable" "LIBDWARF_ROOT variable" "local installation")

  # Search also the subfolders of the installation directory for local installations
  for libdwarf_path in `find $INSTALL_DIR -mindepth 1 -maxdepth 1 -type d -iname "dwarf*"`; do
    libdwarf_paths+=("$libdwarf_path")
    libdwarf_paths_desc+=("local installation ($libdwarf_path)")
  done

  print_subsection "checking libdwarf library"

  # Check if CMake is available, without it, we cannot perform the checks
  if [ -z "$CMAKE" ]; then
    print_info "     checking failed, CMake not found"

    return 1
  fi

  # Try to find the libdwarf library
  for index in ${!libdwarf_paths[@]}; do
    print_info "     checking ${libdwarf_paths_desc[$index]}... " -n

    # Prepare a temporary directory for storing files produced during the check
    mkdir -p $check_libdwarf_temp_dir && cd $check_libdwarf_temp_dir

    # Prepare a CMake script which checks the availability of libdwarf library
    echo -e "
      cmake_minimum_required(VERSION 2.8.3)
      set(CMAKE_MODULE_PATH \"$SOURCE_DIR/shared/cmake\")
      find_package(libdwarf REQUIRED)
      set(LIBDWARF_REQUIRED_INTERNAL_HEADERS
        libdwarf/config.h
        libdwarf/dwarf_alloc.h
        libdwarf/dwarf_base_types.h
        libdwarf/dwarf_opaque.h
        libdwarf/libdwarfdefs.h)
      include(CheckHeaderExists)
      foreach(HEADER \${LIBDWARF_REQUIRED_INTERNAL_HEADERS})
        CHECK_HEADER_EXISTS(\${HEADER} HEADER_DIR REQUIRED PATHS
          \$ENV{LIBDWARF_HOME} \$ENV{LIBDWARF_ROOT} PATH_SUFFIXES include)
        unset(HEADER_DIR)
      endforeach(HEADER)
    " > CMakeLists.txt

    # Use CMake to find the libdwarf library (and its required headers)
    OUTPUT=`LIBDWARF_HOME="${libdwarf_paths[$index]}" $CMAKE . CMakeLists.txt 2>&1`

    # Get the results (exit code first, local is also a command with exit code)
    local libdwarf_search_result=$?
    local libdwarf_info=$OUTPUT

    # Clean everything up
    cd .. && rm -rf $check_libdwarf_temp_dir

    # Check if the libdwarf library was found
    if [ $libdwarf_search_result -eq 0 ]; then
      print_info "found"

      # Extract and save the home directory of the libdwarf library
      local libdwarf_home=`echo "$libdwarf_info" | grep -E "^-- Found libdwarf: .*$" | sed -e "s/^-- Found libdwarf: \(.*\)$/\1/" | sed -e "s/[[:space:]]*$//"`
      env_update_var LIBDWARF_HOME "$libdwarf_home"

      return 0
    else
      print_info "not found"

      if [ $VERBOSE -eq 1 ]; then
        echo -e "\n$libdwarf_info\n"
      fi
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds libdwarf library from its sources in the current directory.
# Parameters:
#   None
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_libdwarf()
{
  print_subsection "building libdwarf library"

  # Download the archive containing the libdwarf library source code
  print_info "     downloading... $LIBDWARF_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$LIBDWARF_STABLE_URL} || terminate "cannot download libdwarf source archive."

  # Extract the source code
  print_info "     extracting... $LIBDWARF_STABLE_TGZ"
  tar xf ./$LIBDWARF_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $LIBDWARF_STABLE_DIR"
  cd $LIBDWARF_STABLE_DIR/libdwarf

  if [ "$HOST_OS" == "linux" ]; then
    CFLAGS="-I$LIBELF_HOME" LDFLAGS="-L$LIBELF_HOME" ./configure --enable-shared || terminate "cannot build libdwarf library."
  else
    ./configure --enable-shared || terminate "cannot build libdwarf library."
  fi

  make || terminate "cannot build libdwarf library."
  cd ../..

  # Update the environment
  env_update_var LIBDWARF_HOME "$BUILD_DIR/$LIBDWARF_STABLE_DIR"
}

#
# Description:
#   Checks if there exist libelf library that meets the version requirements.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if a suitable libelf library was found, 1 otherwise.
#
check_libelf()
{
  # Helper variables
  local check_libelf_temp_dir="./libelf-check"
  local index

  # List of libelf paths to check together with their description
  local libelf_paths=("$LIBELF_HOME" "$LIBELF_ROOT" "$INSTALL_DIR")
  local libelf_paths_desc=("LIBELF_HOME variable" "LIBELF_ROOT variable" "local installation")

  # Search also the subfolders of the installation directory for local installations
  for libelf_path in `find $INSTALL_DIR -mindepth 1 -maxdepth 1 -type d -iname "elfutils*"`; do
    libelf_paths+=("$libelf_path")
    libelf_paths_desc+=("local installation ($libelf_path)")
  done

  print_subsection "checking libelf library"

  # Check if CMake is available, without it, we cannot perform the checks
  if [ -z "$CMAKE" ]; then
    print_info "     checking failed, CMake not found"

    return 1
  fi

  # Try to find the libelf library
  for index in ${!libelf_paths[@]}; do
    print_info "     checking ${libelf_paths_desc[$index]}... " -n

    # Prepare a temporary directory for storing files produced during the check
    mkdir -p $check_libelf_temp_dir && cd $check_libelf_temp_dir

    # Prepare a CMake script which checks the availability of libelf library
    echo -e "
      cmake_minimum_required(VERSION 2.8.3)
      set(CMAKE_MODULE_PATH \"$SOURCE_DIR/shared/cmake\")
      find_package(libelf REQUIRED)
      set(LIBELF_REQUIRED_INTERNAL_HEADERS
        gelf.h)
      include(CheckHeaderExists)
      foreach(HEADER \${LIBELF_REQUIRED_INTERNAL_HEADERS})
        CHECK_HEADER_EXISTS(\${HEADER} HEADER_DIR REQUIRED PATHS
          \$ENV{LIBELF_HOME} \$ENV{LIBELF_ROOT} PATH_SUFFIXES include)
        unset(HEADER_DIR)
      endforeach(HEADER)
    " > CMakeLists.txt

    # Use CMake to find the libelf library (and its required headers)
    OUTPUT=`LIBELF_HOME="${libelf_paths[$index]}" $CMAKE . CMakeLists.txt 2>&1`

    # Get the results (exit code first, local is also a command with exit code)
    local libelf_search_result=$?
    local libelf_info=$OUTPUT

    # Clean everything up
    cd .. && rm -rf $check_libelf_temp_dir

    # Check if the libelf library was found
    if [ $libelf_search_result -eq 0 ]; then
      print_info "found"

      # Extract and save the home directory of the libelf library
      local libelf_home=`echo "$libelf_info" | grep -E "^-- Found libelf: .*$" | sed -e "s/^-- Found libelf: \(.*\)$/\1/" | sed -e "s/[[:space:]]*$//"`
      env_update_var LIBELF_HOME "$libelf_home"

      return 0
    else
      print_info "not found"

      if [ $VERBOSE -eq 1 ]; then
        echo -e "\n$libelf_info\n"
      fi
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Builds libelf library from its sources in the current directory.
# Parameters:
#   None
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_libelf()
{
  print_subsection "building libelf library"

  # Check if the m4 macro processor is available (needed to build libelf)
  print_info "     checking macro processor... " -n

  local m4_version=`m4 --version 2>&1 | grep -o -E "m4 .* [0-9.]+" | grep -o -E "[0-9.]+$"`

  if [ ! -z "$m4_version" ]; then
    print_info "m4 $m4_version"
  else
    print_info "not found"

    terminate "no macro processor found, please install m4."
  fi

  # Download the archive containing the libelf library source code
  print_info "     downloading... $LIBELF_STABLE_URL"
  ${DOWNLOAD_COMMAND//%u/$LIBELF_STABLE_URL} || terminate "cannot download libelf source archive."

  # Extract the source code
  print_info "     extracting... $LIBELF_STABLE_TGZ"
  tar xf ./$LIBELF_STABLE_TGZ

  # Compile the source code
  print_info "     compiling... $LIBELF_STABLE_DIR"
  cd $LIBELF_STABLE_DIR
  ./configure || terminate "cannot build libelf library."
  make || terminate "cannot build libelf library."
  cd ..

  # Update the environment
  env_update_var LIBELF_HOME "$BUILD_DIR/$LIBELF_STABLE_DIR/libelf"
}

#
# Description:
#   Checks if there exist any version of the ANaConDA framework.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if the ANaConDA framework was found, 1 otherwise.
#
check_anaconda_framework()
{
  # Helper variables
  local index

  # List of ANaConDA directories to check together with their description
  local framework_dirs=("$INSTALL_DIR" "$SOURCE_DIR")
  local framework_dirs_desc=("installation directory" "source directory")

  print_subsection "checking ANaConDA framework"

  # Try to find any version of the ANaConDA framework
  for index in ${!framework_dirs[@]}; do
    print_info "     checking ${framework_dirs_desc[$index]}... " -n

    local framework_paths=`find ${framework_dirs[$index]} -regex "${framework_dirs[$index]}/lib/\(ia32\|intel64\)/anaconda-framework\(\.dll\|\.so\)"`

    if [ ! -z "$framework_paths" ]; then
      print_info "found"

      env_update_var ANACONDA_FRAMEWORK_HOME "${framework_dirs[$index]}"

      return 0
    else
      print_info "not found"
    fi
  done

  return 1 # No suitable version found
}

#
# Description:
#   Checks if there exist any ANaConDA analysers.
# Parameters:
#   None
# Output:
#   Detailed information about the checks performed.
# Return:
#   0 if any ANaConDA analyser was found, 1 otherwise.
#
check_anaconda_analysers()
{
  # Helper variables
  local index
  local analyser

  # List of ANaConDA directories to check together with their description
  local analysers_dirs=("$INSTALL_DIR" "$SOURCE_DIR")
  local analysers_dirs_desc=("installation directory" "source directory")

  print_subsection "checking ANaConDA analysers"

  # Try to find all ANaConDA analysers available
  for index in ${!analysers_dirs[@]}; do
    print_info "     checking ${analysers_dirs_desc[$index]}..." -n

    local analysers_found=`find ${analysers_dirs[$index]} -regex "${analysers_dirs[$index]}/lib/\(ia32\|intel64\)/anaconda-.*\(\.dll\|\.so\)" | sed -e 's#.*/lib/\(ia32\|intel64\)/anaconda-\(.*\)\(\.dll\|\.so\)#\2#' | grep -v framework | sort -u`

    if [ ! -z "$analysers_found" ]; then
      for analyser in $analysers_found; do
        print_info " $analyser" -n

        # Generate a prefix for the environment variable from the analyser name
        local analyser_prefix=`echo "${analyser//-/_}" | tr '[:lower:]' '[:upper:]'`

        env_update_var ANACONDA_${analyser_prefix}_HOME "${analysers_dirs[$index]}"
      done

      print_info ""

      local analyser_found=1 # We found at least one analyser
    else
      print_info " none found"
    fi
  done

  if [ ! -z "$analyser_found" ]; then
    return 0 # At least one analyser was found
  else
    return 1 # No suitable version found
  fi
}

#
# Description:
#   Builds a target from its sources.
# Parameters:
#   [STRING] A name of a directory in the source directory which contains the
#            source files needed to build the target.
# Output:
#   Detailed information about the build process.
# Return:
#   Nothing
#
build_target()
{
  # Helper variables
  local target_name="$1"

  # Target-specific configuration (prefix of environment variables, etc.)
  case "$target_name" in
    analysers/*)
      local analyser_name=`echo "${target_name%/}" | sed -e "s/^analysers\/\(.*\)$/\1/"`
      local target_prefix=anaconda-${analyser_name}
      local target_include_subdir=anaconda/${analyser_name}
      ;;
    libraries/*)
      local library_name=`echo "${target_name%/}" | sed -e "s/^libraries\/\(.*\)$/\1/"`
      local target_prefix=${library_name}
      local target_include_subdir=${library_name}
      ;;
    wrappers/*)
      local wrapper_name=`echo "${target_name%/}" | sed -e "s/^wrappers\/\(.*\)$/\1/"`
      local target_prefix=${wrapper_name}-wrapper
      local target_include_subdir=${wrapper_name}-wrapper
      ;;
    *) # Framework
      local target_prefix=anaconda-${target_name%/}
      local target_include_subdir=anaconda
      ;;
  esac

  # Convert the prefix to the upper case format with '-' replaced by '_'
  local target_prefix=`echo "${target_prefix//-/_}" | tr '[:lower:]' '[:upper:]'`

  # Build the target
  print_subsection "building ${target_name%/}"

  # Copy the source files from the source directory to the build directory
  print_info "     copying source files to build directory... " -n

  if [ -d "$SOURCE_DIR/$target_name" ]; then
    # Create the directory hierarchy if any
    local dirs=`dirname ${target_name%/}`
    mkdir -p ./$dirs

    if [ "$HOST_OS" == "mac" ]; then
      # Use rsync on Mac OS X, as its cp does not support update
      local dir_update_command="rsync -ur"
    else
      # Use standard copy on other systems as it supports update
      local dir_update_command="cp -uR"
    fi

    # Copy the files to the right directory
    $dir_update_command "$SOURCE_DIR/$target_name" ./$dirs
    # Copy the files used by all targets
    $dir_update_command "$SOURCE_DIR/shared" .
    # Copy the files used by tests
    $dir_update_command "$SOURCE_DIR/tests" .

    print_info "done"
  else
    print_info "failed"

    terminate "directory $SOURCE_DIR/$target_name not found."
  fi

  # Out-of-source builds only
  if [ "$BUILD_DIR" != "$SOURCE_DIR" ]; then
    # Fix or create references to the Git repository of the target
    if [ -f "$SOURCE_DIR/${target_name%/}/.git" ]; then
      # Some reference to the Git repository already exits, fix it if needed
      print_info "     fixing Git repository reference... " -n

      # Get the path to the referenced Git repository
      local gitdir=`cat $SOURCE_DIR/${target_name%/}/.git | sed "s/gitdir:[ ]\+\(.*\)/\1/"`

      # If the path is relative, replace it with an absolute path
      if [[ "$gitdir" =~ ^[.].*$ ]]; then
        echo -n "gitdir: $SOURCE_DIR/${target_name%/}/$gitdir" \
          > "$BUILD_DIR/${target_name%/}/.git"
      fi

      print_info "done"
    elif [ -d "$SOURCE_DIR/.git" ]; then
      # No reference to the Git repository exits yet, create a new one
      print_info "     creating Git repository reference... " -n

      # Target is part of ANaConDA, create a reference to its Git repository
      echo -n "gitdir: $SOURCE_DIR/.git" > "$BUILD_DIR/${target_name%/}/.git"

      print_info "done"
    fi
  fi

  cd $target_name

  # Remove the target before the compilation if requested
  if [ "$CLEAN" == "1" ]; then
    make $MAKE_FLAGS uninstall distclean \
      || terminate "cannot clean $target_name."
  fi

  # Compile the target
  make $MAKE_FLAGS $BUILD_TYPE INSTALL_DIR="$INSTALL_DIR" \
    INSTALL_INCLUDEDIR="include/$target_include_subdir" \
    INSTALL_LIBDIR="lib/$PIN_TARGET_LONG" \
    || terminate "cannot build $target_name."

  # Install the target
  make $MAKE_FLAGS install || terminate "cannot install $target_name."

  cd $BUILD_DIR

  # Update the environment
  env_update_var "${target_prefix}_HOME" "$INSTALL_DIR"
}

#
# Description:
#   Tests a target.
# Parameters:
#   [STRING] A name of a directory in the source directory which contains the
#            target files.
# Output:
#   Detailed information about the test process.
# Return:
#   Nothing
#
test_target()
{
  # Helper variables
  local target_name="$1"
  local test_flags="$MAKE_FLAGS"

  # Test the target
  print_subsection "testing ${target_name%/}"

  # Build all programs needed to test the target
  print_info "     building programs needed to test ${target_name%/}... "

  cd $target_name

  make $test_flags build-tests \
    || terminate "cannot build programs needed to test ${target_name%/}."

  # Execute all tests
  print_info "     executing tests... "

  # Print the output of failed tests
  if [ "$VERBOSE" == "1" ]; then
    test_flags="$test_flags CTEST_OUTPUT_ON_FAILURE=1"
  fi

  make $test_flags test || terminate "cannot test ${target_name%/}."

  cd $BUILD_DIR
}

#
# Description:
#   Cleans a target.
# Parameters:
#   [STRING] A name of a directory in the source directory which contains the
#            target files.
# Output:
#   Detailed information about the clean process.
# Return:
#   Nothing
#
clean_target()
{
  # Helper variables
  local target_name="$1"

  # Clean the target
  print_subsection "cleaning ${target_name%/}"

  # Do the cleanup only when there is anything to clean
  if [ -f "$BUILD_DIR/$target_name/Makefile" ]; then
    cd $BUILD_DIR/$target_name
    make $MAKE_FLAGS uninstall distclean \
      || terminate "cannot clean ${target_name%/}."
    cd $BUILD_DIR
  fi

  # Out-of-source builds only
  if [ "$BUILD_DIR" != "$SOURCE_DIR" ] && [ -d "$BUILD_DIR" ]; then
    # Remove files copied to the build directory if build files are cleaned
    if [ ! -d "$BUILD_DIR/${target_name%/}/build" ]; then
      #
      # Does a reverse operation to copying source files to the build directory,
      # i.e., removes all files in the build directory that are also present in
      # the source directory.
      #
      clean_target_sources()
      { # We need paths to the files that are relative to the source directory
        pushd $SOURCE_DIR > /dev/null

        # Remove the Git repository reference created during the build first
        rm -f "$BUILD_DIR/$1/.git"

        # Remove files found in the source directory from the build directory
        for file in `find ./$1/ -name "*" -type f | sed 's#^\./##'`; do
          rm -f "$BUILD_DIR/$file"
        done

        # Remove folders found in the source directory from the build directory
        for folder in `find ./$1/ -name "*" -type d | sed 's#^\./##'`; do
          if [ -d "$BUILD_DIR/$folder" ]; then
            rmdir -p --ignore-fail-on-non-empty "$BUILD_DIR/$folder"
          fi
        done

        # Return to the initial directory
        popd > /dev/null
      }

      # Clean the target's source files
      if [ -d "$BUILD_DIR/${target_name%/}" ]; then
        print_info "     removing source files from build directory... " -n

        clean_target_sources ${target_name%/}

        print_info "done"
      fi

      # Clean the target's tests
      if [ -d "$BUILD_DIR/tests/${target_name%/}" ]; then
        print_info "     removing tests from build directory... " -n

        clean_target_sources "tests/${target_name%/}"

        print_info "done"
      fi

      # Clean source files shared by all targets if all targets were cleaned
      if [ -d "$BUILD_DIR" ]; then
        # We need paths to the files that are relative to the build directory
        cd $BUILD_DIR

        if [ -z "$(find . -mindepth 1 ! -regex '^\./\(shared\|tests/shared\).*$' -a ! -name 'tests')" ]; then
          print_info "     removing shared files from build directory... " -n
  
          # Move to the source directory as the build directory will be removed
          cd $SOURCE_DIR
  
          clean_target_sources "shared"
          clean_target_sources "tests/shared"
  
          print_info "done"
        fi
      fi
    fi
  fi
}

# Program section
# ---------------

# Default values for optional parameters
CLEAN=0
BUILD_TYPE=release
BUILD_DIR=$SCRIPT_DIR/build
INSTALL_DIR=$SCRIPT_DIR/local
SOURCE_DIR=$SCRIPT_DIR
PREBUILD_ACTION=
ACTION_PARAMS=
VERBOSE=0
TEST=0

# Initialise environment first, optional parameters might override the values
env_init

# Process the optional parameters
until [ -z "$1" ]; do
  case "$1" in
    "-h"|"--help")
      usage
      exit 0
      ;;
    "--clean")
      CLEAN=1
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
    "--source-dir")
      if [ -z "$2" ]; then
        terminate "missing path to the source directory."
      fi
      SOURCE_DIR=$2
      shift
      ;;
    "--check-environment")
      PREBUILD_ACTION=check
      ACTION_PARAMS=build
      ;;
    "--setup-environment")
      PREBUILD_ACTION=setup
      ACTION_PARAMS=build
      ;;
    "--check-runtime")
      PREBUILD_ACTION=check
      ACTION_PARAMS=runtime
      ;;
    "--setup-runtime")
      PREBUILD_ACTION=setup
      ACTION_PARAMS=runtime
      ;;
    "--verbose")
      VERBOSE=1
      ;;
    "--target-arch")
      if [ -z "$2" ]; then
        terminate "missing target architecture."
      fi
      TARGET_ARCH=$2
      shift
      ;;
    "--test")
      TEST=1
      ;;
    *)
      break;
      ;;
  esac

  # Move to the next parameter
  shift
done

# Fix the colision between Cygwin's and Visual Studio's linkers (link.exe)
if [ "$HOST_OS" == "windows" ]; then
  # Extract the the directory containing the Visual Studio linker from PATH
  vc_bin_dir=`echo ":$PATH:" | grep -i -o "[^:]*vc/bin[^:]*"`

  # Search the Visual Studio paths before the Cygwin paths
  PATH=$vc_bin_dir:$PATH
fi

# Process the positional parameters
if [ -z "$1" ]; then
  if [ -z "$PREBUILD_ACTION" ] && [ "$CLEAN" == "0" ]; then
    terminate "no target specified."
  fi
else
  BUILD_TARGET=$1
fi

print_section "Preparing build script..."

print_script_info $(basename ${BASH_SOURCE[0]})

print_subsection "checking build settings"

print_info "     build type... " -n

if [[ "$BUILD_TYPE" =~ ^release$|^debug$ ]]; then
  print_info "$BUILD_TYPE"
else
  print_info "invalid"
  terminate "build type must be release or debug."
fi

print_info "     target architecture... " -n

if [ -z "$TARGET_ARCH" ]; then
  if [ "$HOST_OS" == "windows" ]; then
    # On Windows, derive the target architecture from compiler or OS
    if [ -f "`which cl`" ]; then
      # Deriving from compiler is better as we may be cross-compiling
      TARGET_ARCH=`cl /? 2>&1 | head -1 | sed "s/^.*\(x[0-9]\+\)[\r]*$/\1/"`
    else
      # If no compiler is present, we are probably preparing runtime
      TARGET_ARCH="$PROCESSOR_ARCHITECTURE"
    fi
  else
    # On Linux, derive the target architecture from the running OS
    TARGET_ARCH=`uname -m`
  fi
fi

case "$TARGET_ARCH" in
  "x86_64"|"amd64"|"x64")
    print_info "$TARGET_ARCH"
    TARGET_ARCH=x86_64
    PIN_TARGET_LONG=intel64
    ;;
  "x86"|"i686"|"i386")
    print_info "$TARGET_ARCH"
    TARGET_ARCH=x86
    PIN_TARGET_LONG=ia32
    ;;
  *)
    print_info "invalid"
    terminate "architecture $TARGET_ARCH is not supported."
    ;;
esac

print_info "     build directory... " -n

if [ -d "$BUILD_DIR" ]; then
  BUILD_DIR=$(cd "$BUILD_DIR" && pwd)

  print_info "$BUILD_DIR"
else
  if mkdir -p "$BUILD_DIR"; then
    BUILD_DIR=$(cd "$BUILD_DIR" && pwd)

    print_info "$BUILD_DIR"
  else
    print_info "not found"
    terminate "directory $BUILD_DIR cannot be created."
  fi
fi

env_update_var BUILD_DIR "$BUILD_DIR"

print_info "     installation directory... " -n

if [ -d "$INSTALL_DIR" ]; then
  INSTALL_DIR=$(cd "$INSTALL_DIR" && pwd)

  print_info "$INSTALL_DIR"
else
  if mkdir -p "$INSTALL_DIR"; then
    INSTALL_DIR=$(cd "$INSTALL_DIR" && pwd)

    print_info "$INSTALL_DIR"
  else
    print_info "not found"
    terminate "directory $INSTALL_DIR cannot be created."
  fi
fi

env_update_var INSTALL_DIR "$INSTALL_DIR"

print_info "     source directory... " -n

if [ -d "$SOURCE_DIR" ]; then
  SOURCE_DIR=$(cd "$SOURCE_DIR" && pwd)

  print_info "$SOURCE_DIR"
else
  print_info "not found"
  terminate "directory $SOURCE_DIR not found."
fi

env_update_var SOURCE_DIR "$SOURCE_DIR"

print_subsection "configuring build script"

print_info "     download command... " -n

# Setup the command used to download files from the internet or local network
if [ -z "$DOWNLOAD_COMMAND" ]; then
  # If no download command is set, check if wget is available
  WGET_VERSION=`wget -V 2>&1 | grep -o -E "Wget.[0-9.]+"`

  if [ "$WGET_VERSION" != "" ]; then
    DOWNLOAD_COMMAND="wget --passive-ftp --no-check-certificate -c %u"
  fi
fi

print_info "$DOWNLOAD_COMMAND"

cd $BUILD_DIR

# Execute all requested pre-build actions
if [ "$PREBUILD_ACTION" == "setup" ]; then
  print_section "Setting up $ACTION_PARAMS environment..."

  if [ "$HOST_OS" == "windows" ]; then
    # On Windows, we need to setup CMake, Boost and PIN (VS is already set up)
    if [ "$ACTION_PARAMS" == "build" ]; then
      if ! check_cmake; then
        install_cmake
      fi

      if ! check_boost; then
        install_boost
      fi
    fi

    # As for the runtime environment, only PIN is needed to run ANaConDA
    if ! check_pin; then
      install_pin
    fi

    # If setting up the runtime environment, setup paths to the ANaConDA
    if [ "$ACTION_PARAMS" == "runtime" ]; then
      check_anaconda_framework
      check_anaconda_analysers
    fi
  elif [ "$HOST_OS" == "linux" ]; then
    # On Linux, we need to setup GCC, CMake, Boost, PIN, libdwarf and libelf
    if ! check_gcc; then
      build_gcc
    fi

    # Prefer the GCC we found before the others
    switch_gcc $GCC_HOME

    if ! check_cmake; then
      build_cmake
    fi

    if ! check_boost; then
      build_boost
    fi

    if ! check_pin; then
      install_pin
    fi

    if ! check_libelf; then
      build_libelf
    fi

    if ! check_libdwarf; then
      build_libdwarf
    fi
  elif [ "$HOST_OS" == "mac" ]; then
    # On Mac OS X, we need to setup CMake, Boost and PIN
    if ! check_cmake; then
      build_cmake
    fi

    if ! check_boost; then
      build_boost
    fi

    if ! check_pin; then
      install_pin
    fi
  else
    terminate "unsupported operating system."
  fi
elif [ "$PREBUILD_ACTION" == "check" ]; then
  print_section "Checking $ACTION_PARAMS environment..."

  if [ "$HOST_OS" == "windows" ]; then
    # On Windows, we need to check CMake, Boost and PIN (VS was checked before)
    if [ "$ACTION_PARAMS" == "build" ]; then
      check_cmake
      check_boost
    fi

    # As for the runtime environment, only PIN is needed to run ANaConDA
    check_pin

    # If checking for runtime, check if ANaConDA is already available
    if [ "$ACTION_PARAMS" == "runtime" ]; then
      check_anaconda_framework
      check_anaconda_analysers
    fi
  else
    # On Linux, we need to check GCC, CMake, Boost, PIN, libdwarf and libelf
    if check_gcc; then
      switch_gcc $GCC_HOME
    fi

    check_cmake
    check_boost
    check_pin
    check_libelf
    check_libdwarf
  fi
fi

# Make parameters shared by all build targets
MAKE_FLAGS=

# Set the target architecture or PIN will choose it based on the OS arch
MAKE_FLAGS="$MAKE_FLAGS HOST_ARCH=$PIN_TARGET_LONG"

# Use separate build directories for different architectures and OSes
MAKE_FLAGS="$MAKE_FLAGS BUILD_DIR=build/$HOST_OS/$PIN_TARGET_LONG"

# Enable the verbose mode if requested
if [ "$VERBOSE" == "1" ]; then
  MAKE_FLAGS="$MAKE_FLAGS VERBOSE=1"
fi

if [ ! -z "$BUILD_TARGET" ]; then
  if [ "$HOST_OS" != "windows" ]; then
    # Check GCC before building targets
    print_info "     compiler... " -n

    if [ -f "`which g++`" ]; then
      print_info "g++"
    else
      print_info "not found"

      terminate "no compiler to build the target, use --setup-environment to install it."
    fi

    # Setup GCC before building targets
    switch_gcc $GCC_HOME
  fi

  print_section "Building $BUILD_TARGET..."
elif [ "$CLEAN" == "1" ]; then
  print_section "Cleaning all targets..."

  # Remove build and installation directories if they are empty
  cd $SOURCE_DIR
  rmdir -p --ignore-fail-on-non-empty "$BUILD_DIR"
  rmdir -p --ignore-fail-on-non-empty "$INSTALL_DIR"

  clean_target libraries/libdie
  clean_target wrappers/libdie
  clean_target framework

  for analyser in `find $SOURCE_DIR/analysers -mindepth 1 -maxdepth 1 -type d`; do
    clean_target ${analyser/$SOURCE_DIR\//}
  done
fi

# Build the target(s)
case "$BUILD_TARGET" in
  all)
    build_target libraries/libdie
    build_target wrappers/libdie
    build_target framework

    for analyser in `find $SOURCE_DIR/analysers -mindepth 1 -maxdepth 1 -type d`; do
      build_target ${analyser/$SOURCE_DIR\//}
    done
    ;;
  anaconda)
    build_target libraries/libdie
    build_target wrappers/libdie
    build_target framework
    ;;
  analysers)
    for analyser in `find $SOURCE_DIR/analysers -mindepth 1 -maxdepth 1 -type d`; do
      build_target ${analyser/$SOURCE_DIR\//}
    done
    ;;
  "")
    ;;
  *)
    build_target "$BUILD_TARGET"

    if [ "$TEST" == "1" ]; then
      test_target "$BUILD_TARGET"
    fi
    ;;
esac

# Move back to the directory in which we executed the script
cd $SCRIPT_DIR

# End of script
