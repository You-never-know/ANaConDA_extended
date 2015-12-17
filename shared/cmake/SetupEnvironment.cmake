#
# A CMake module which sets up the build environment.
#
# File:      SetupEnvironment.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2015-12-17
# Date:      Last Update 2015-12-17
# Version:   0.1
#

# Determine the build type, only one build type can be set at a time, if more
# are specified, keep the one with the highest priority only, priority list:
#   1) DEBUG   (highest priority)
#   2) CHECKED
#   3) RELEASE (lowest priority)
if (DEBUG)
  set (CHECKED FALSE)
  set (RELEASE FALSE)
elseif (CHECKED)
  set (DEBUG FALSE)
  set (RELEASE FALSE)
else (DEBUG)
  set (DEBUG FALSE)
  set (CHECKED FALSE)
  # If no build type is specified, presume the RELEASE build type
  set (RELEASE TRUE)
endif (DEBUG)

# Determine if building for the host or target (crosscompiling)
if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  if ("${TARGET_LONG}" STREQUAL "ia32")
    set (CROSSCOMPILING_32_ON_64 TRUE)
  else ("${TARGET_LONG}" STREQUAL "ia32")
    set (CROSSCOMPILING_32_ON_64 FALSE)
  endif ("${TARGET_LONG}" STREQUAL "ia32")
endif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

# End of file SetupEnvironment.cmake
