#
# Copyright (C) 2015-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
# A CMake module which sets up the build environment.
#
# File:      SetupEnvironment.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2015-12-17
# Date:      Last Update 2019-01-28
# Version:   0.1.1
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

# Determine if building for the host or target (cross-compiling)
if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|AMD)64$")
  if ("${TARGET_LONG}" STREQUAL "ia32")
    set (CROSS_COMPILING_32_ON_64 TRUE)
  else ("${TARGET_LONG}" STREQUAL "ia32")
    set (CROSS_COMPILING_32_ON_64 FALSE)
  endif ("${TARGET_LONG}" STREQUAL "ia32")
endif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_|AMD)64$")

# End of file SetupEnvironment.cmake
