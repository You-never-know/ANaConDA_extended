cmake_minimum_required(VERSION 2.8.3)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
find_package(libdwarf REQUIRED)
set(LIBDWARF_REQUIRED_INTERNAL_HEADERS
  libdwarf/config.h
  libdwarf/dwarf_alloc.h
  libdwarf/dwarf_base_types.h
  libdwarf/dwarf_opaque.h
  libdwarf/libdwarfdefs.h)
include(CheckHeaderExists)
foreach(HEADER ${LIBDWARF_REQUIRED_INTERNAL_HEADERS})
  CHECK_HEADER_EXISTS(${HEADER} HEADER_DIR REQUIRED PATHS $ENV{LIBDWARF_HOME}
    $ENV{LIBDWARF_ROOT} PATH_SUFFIXES include)
  unset(HEADER_DIR)
endforeach(HEADER)