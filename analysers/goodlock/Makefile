#
# GoodLock Plugin General Makefile
#
# File:      Makefile
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2012-03-09
# Date:      Last Update 2014-10-02
# Version:   0.2
#

# We need to tell the PIN's Makefile if we are compiling a debug version or not
ifeq ($(findstring debug, $(MAKECMDGOALS)), debug)
  DEBUG=1
endif

# We need to get the location, defines and other info about the PIN framework 
include ../../shared/Makefile.pin

# Build the IA-32, Intel64 and Itanium versions of the plugin separately
BUILD_DIR = ./build/$(HOST_ARCH)

# The CMake executable we will use to perform the build 
CMAKE ?= cmake
# We need to pass some information set by the PIN's Makefile to CMake
CMAKE_PIN_FLAGS  = -DTARGET_LONG="$(HOST_ARCH)"
CMAKE_PIN_FLAGS += -DCXX="$(CXX)"
CMAKE_PIN_FLAGS += -DPIN_CXXFLAGS="$(TOOL_CXXFLAGS)"
CMAKE_PIN_FLAGS += -DPIN_LDFLAGS="$(TOOL_LDFLAGS)"
CMAKE_PIN_FLAGS += -DPIN_LPATHS="$(TOOL_LPATHS)"
CMAKE_PIN_FLAGS += -DPIN_LIBS="$(TOOL_LIBS)"
# General CMake parameters used in all types of build (debug, release, etc.)
CMAKE_FLAGS = -DCMAKE_INSTALL_PREFIX=../.. $(CMAKE_PIN_FLAGS)

# The default build type is a release version
all: release

# Debug version of the plugin
debug:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DDEBUG=1 ../..
	$(MAKE) -C $(BUILD_DIR)

# Release version of the plugin
release:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DRELEASE=1 ../..
	$(MAKE) -C $(BUILD_DIR)

# Installation of the plugin
install:
	$(MAKE) -C $(BUILD_DIR) install

# Cleanup of the compiled objects, Makefiles and other stuff created by CMake
clean:
	rm -rf ./build ./include ./lib

# End of file Makefile
