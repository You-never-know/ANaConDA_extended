#
# A CMake module which sets up the Boost library.
#
# File:      SetupBoost.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2015-05-29
# Date:      Last Update 2015-08-13
# Version:   0.2.4
#

#
# Sets up the Boost library for a specific project. This includes finding the
#   Boost header files and required components (specific parts of the library)
#   and configuring the compiler and linker flags to be able to compile the
#   project with the version of Boost library found.
#
# SETUP_BOOST(<project> <version> [<component1> [<component2> ...]])
#
MACRO(SETUP_BOOST project version)
  # Only BOOST_ROOT is searched, if not set, use the path in BOOST_HOME
  if ("$ENV{BOOST_ROOT}" STREQUAL "")
    set(ENV{BOOST_ROOT} "$ENV{BOOST_HOME}")
  endif ("$ENV{BOOST_ROOT}" STREQUAL "")

  # Windows only, correct Cygwin paths to Windows paths
  if (WIN32)
    # Load the module for correcting paths
    include(Paths)
    # Correct the paths to Boost if necessary
    CORRECT_PATHS(ENV{BOOST_HOME} ENV{BOOST_ROOT})
    # Determine the version of Visual Studio from the MSVC compiler version
    math(EXPR VS_VERSION_MAJOR "${MSVC_VERSION} / 100 - 6")
    math(EXPR VS_VERSION_MINOR "${MSVC_VERSION} % 100 / 10")
    set(VS_VERSION "${VS_VERSION_MAJOR}.${VS_VERSION_MINOR}")
    # Determine the target architecture (32-bit or 64-bit) from the compiler
    math(EXPR TARGET_ARCH_BITS "32 + 32 * ${CMAKE_CL_64}")
    # Add path to the pre-compiled Boost libraries to the library search path
    set(ENV{BOOST_LIBRARYDIR} "$ENV{BOOST_ROOT}/lib${TARGET_ARCH_BITS}-msvc-${VS_VERSION}")
  endif (WIN32)

  # Multi-threaded version is safer and often the only one available on Windows
  set(Boost_USE_MULTITHREADED TRUE)
  # If possible, all libraries used by pintools should be static libraries
  if (UNIX)
    # On Linux, static linking fails, use dynamic libs until resolved
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
  else (UNIX)
    # On Windows, PIN requires all libraries to be linked statically
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
  endif (UNIX)
  # Find the Boost library and the required components (libraries)
  find_package(Boost ${version} REQUIRED COMPONENTS ${ARGN})

  # Add the found header files to the compiler flags
  include_directories(${Boost_INCLUDE_DIRS})
  # Add the found components to the linker flags
  target_link_libraries(${project} ${Boost_LIBRARIES})

  # Print the directories where the header files and components were found
  message("-- Boost header files: "${Boost_INCLUDE_DIRS})
  message("-- Boost libraries paths: ")
  # The list of components contains both debug and release (optimized) versions
  set(SKIP_COMPONENT_PATH NO)
  foreach(COMPONENT_PATH ${Boost_LIBRARIES})
    if ("${COMPONENT_PATH}" STREQUAL "optimized")
      if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(SKIP_COMPONENT_PATH YES)
      endif (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    elseif ("${COMPONENT_PATH}" STREQUAL "debug")
      if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(SKIP_COMPONENT_PATH YES)
      endif (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    else ("${COMPONENT_PATH}" STREQUAL "optimized")
      if (NOT SKIP_COMPONENT_PATH)
        message("     "${COMPONENT_PATH})
      else (NOT SKIP_COMPONENT_PATH)
        set(SKIP_COMPONENT_PATH NO)
      endif (NOT SKIP_COMPONENT_PATH)
    endif ("${COMPONENT_PATH}" STREQUAL "optimized")
  endforeach(COMPONENT_PATH ${Boost_LIBRARIES})
ENDMACRO(SETUP_BOOST)

# End of file SetupBoost.cmake
