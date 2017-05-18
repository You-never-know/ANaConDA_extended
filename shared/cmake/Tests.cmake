#
# A CMake module which automates testing.
#
# File:      Tests.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2016-03-24
# Date:      Last Update 2017-05-18
# Version:   0.11
#

# Enable commands for defining tests 
enable_testing()

# A target which compiles test programs
add_custom_target(build-tests)

# A directory used to perform the tests
set(TEST_WORK_DIR test)

# Executables have different extensions on different systems
if (WIN32)
  set(TEST_PROGRAM_EXT ".exe")
else (WIN32)
  set(TEST_PROGRAM_EXT "")
endif (WIN32)

#
# Loads a test configuration.
#
# LOAD_TEST_CONFIG(<test>)
#
macro(LOAD_TEST_CONFIG TEST)
  # Get the name of the configuration file (without the .conf extension)
  get_filename_component(TEST_CONFIG_NAME ${TEST} NAME)

  # Load the test configuration, ignore invalid entries and sections
  file(STRINGS "${TEST_DIR}/${TEST}/${TEST_CONFIG_NAME}.conf" CONFIG_ENTRIES
    REGEX "^(([^=]*=.*)|(\\[.*\\]))$")

  # A name of the currently processed section of the configuration file
  set(CURRENT_CONFIG_SECTION "general")

  # Process all (valid) entries (and sections) of the configuration file
  foreach (CONFIG_ENTRY ${CONFIG_ENTRIES})
    # Check if we are entering some section of the configuration file
    if ("${CONFIG_ENTRY}" MATCHES "^\\[.*\\]$")
      # Extract the name of the section we are currently entering
      string(REGEX REPLACE "^\\[(.*)\\]$" "\\1" CONFIG_SECTION
        "${CONFIG_ENTRY}")

      # Set the section as the currently processed section
      set(CURRENT_CONFIG_SECTION "${CONFIG_SECTION}")

      # Continue with the next configuration entry
      continue()
    endif ("${CONFIG_ENTRY}" MATCHES "^\\[.*\\]$")

    # Ignore configuration entries for a different operating systems
    if (NOT "${CURRENT_CONFIG_SECTION}" STREQUAL "general")
      if ("${CURRENT_CONFIG_SECTION}" STREQUAL "linux")
        if (NOT UNIX)
          # This configuration entry is for different OS, ignore it
          continue()
        endif (NOT UNIX)
      else ("${CURRENT_CONFIG_SECTION}" STREQUAL "linux")
        if ("${CURRENT_CONFIG_SECTION}" STREQUAL "windows")
          if (NOT WIN32)
            # This configuration entry is for different OS, ignore it
            continue()
          endif (NOT WIN32)
        else ("${CURRENT_CONFIG_SECTION}" STREQUAL "windows")
          if ("${CURRENT_CONFIG_SECTION}" STREQUAL "macos")
            if (NOT APPLE)
              # This configuration entry is for different OS, ignore it
              continue()
            endif (NOT APPLE)
          else ("${CURRENT_CONFIG_SECTION}" STREQUAL "macos")
            # Unknown section, ignore all configuration entries in it
            continue()
          endif ("${CURRENT_CONFIG_SECTION}" STREQUAL "macos")
        endif ("${CURRENT_CONFIG_SECTION}" STREQUAL "windows")
      endif ("${CURRENT_CONFIG_SECTION}" STREQUAL "linux")
    endif (NOT "${CURRENT_CONFIG_SECTION}" STREQUAL "general")

    # Split the configuration entry into a key and value pair
    string(REGEX REPLACE "([^=]*)=.*" "\\1" CONFIG_KEY "${CONFIG_ENTRY}")
    string(REGEX REPLACE "[^=]*=(.*)" "\\1" CONFIG_VALUE "${CONFIG_ENTRY}")

    # Convert the key name to a TEST_CONFIG_<upper-case-key-name> format
    string(TOUPPER "TEST_CONFIG_${CONFIG_KEY}" CONFIG_KEY)

    # Save the configuration entry as a key/value pair
    set(${CONFIG_KEY} "${CONFIG_VALUE}")
  endforeach (CONFIG_ENTRY ${CONFIG_ENTRIES})
endmacro(LOAD_TEST_CONFIG)

#
# Setups a test to use configuration stored in a given directory. Configuration
#   files that conflict with the configuration files in this directory will be
#   overwritten!
#
# CONFIGURE_TEST(<test> <config-directory>)
#
macro(CONFIGURE_TEST TEST CONFIG_DIR)
  # This is the directory where are the configuration files stored
  set(SOURCE_CONFIG_DIR "${CONFIG_DIR}")
  # This is the directory where is the test stored
  set(TARGET_TEST_ROOT "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}")
  # Load the module for correcting paths
  include(Paths)
  # Correct the paths to both source and target directories if needed
  CORRECT_PATHS(SOURCE_CONFIG_DIR TARGET_TEST_ROOT)
  # Copy the whole directory with configuration files to the test directory
  file(COPY "${SOURCE_CONFIG_DIR}" DESTINATION "${TARGET_TEST_ROOT}")
endmacro(CONFIGURE_TEST TEST CONFIG_DIR)

#
# Compiles a test program.
#
# COMPILE_TEST_PROGRAM(<test>)
#
macro(COMPILE_TEST_PROGRAM TEST)
  # Clear variables set by the previous calls to the macro
  unset(TEST_SOURCES)

  # Collect source files of the program needed for the test
  aux_source_directory(${TEST_DIR}/${TEST} TEST_SOURCES)

  # Do not compile the test program when building the main program
  add_executable(${TEST} EXCLUDE_FROM_ALL ${TEST_SOURCES})

  # The name of the test may be in the <directory>/<test> format
  get_filename_component(TEST_PROGRAM_DIR ${TEST} DIRECTORY)
  get_filename_component(TEST_PROGRAM_NAME ${TEST} NAME)

  # Additional options passed to the compiler and linker
  set(TEST_PROGRAM_COMPILE_FLAGS "")
  set(TEST_PROGRAM_LINK_FLAGS "")

  # Do not optimize the test program, it may remove tested code
  if (UNIX)
    set(TEST_PROGRAM_COMPILE_FLAGS "${TEST_PROGRAM_COMPILE_FLAGS} -g -O0")
  else (UNIX)
    set(TEST_PROGRAM_COMPILE_FLAGS "${TEST_PROGRAM_COMPILE_FLAGS} /Zi /Od")
  endif (UNIX)

  # Append options specified for the test program in the configuration file
  set(TEST_PROGRAM_COMPILE_FLAGS
    "${TEST_PROGRAM_COMPILE_FLAGS} ${TEST_CONFIG_CFLAGS}")
  set(TEST_PROGRAM_LINK_FLAGS
    "${TEST_PROGRAM_LINK_FLAGS} ${TEST_CONFIG_LDFLAGS}")

  # Store the test program's executable in the test folder
  set_target_properties(${TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}"
    RUNTIME_OUTPUT_NAME "${TEST_PROGRAM_NAME}.test"
    COMPILE_FLAGS "${TEST_PROGRAM_COMPILE_FLAGS}"
    LINK_FLAGS "${TEST_PROGRAM_LINK_FLAGS}")

  # Compile the test program when building test programs
  add_dependencies(build-tests ${TEST})
endmacro(COMPILE_TEST_PROGRAM)

#
# Prepares a test program.
#
# PREPARE_TEST_PROGRAM(<test> [<program>])
#
macro(PREPARE_TEST_PROGRAM TEST)
  if (${ARGC} LESS 2)
    # Using test program which is part of this test
    if (NOT TARGET ${TEST})
      # Test program not compiled yet, do it now
      COMPILE_TEST_PROGRAM(${TEST})
    endif (NOT TARGET ${TEST})
  else (${ARGC} LESS 2)
    # Reusing program compiled as part of another test
    if (NOT TARGET ${ARGV1})
      # Test program not compiled yet, do it now
      COMPILE_TEST_PROGRAM(${ARGV1})
    endif (NOT TARGET ${ARGV1})

    # Both test and test program names can be in <directory>/<name> format
    get_filename_component(TEST_PROGRAM_NAME ${ARGV1} NAME)
    get_filename_component(TEST_NAME ${TEST} NAME)

    # Test name is not a valid target name as it contains slashes
    string(REPLACE "/" "." TEST_TARGET ${TEST})

    # Path to the program we need to use and path to where it is needed to be
    set(REUSED_TEST_PROGRAM_PATH
      "${TEST_DIR}/${ARGV1}/${TEST_WORK_DIR}/${TEST_PROGRAM_NAME}.test${TEST_PROGRAM_EXT}")
    set(TARGET_TEST_PROGRAM_PATH
      "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.test${TEST_PROGRAM_EXT}")

    # Load the module for correcting paths
    include(Paths)
    # Correct the paths to both source and target paths if needed
    CORRECT_PATHS(REUSED_TEST_PROGRAM_PATH TARGET_TEST_PROGRAM_PATH)

    # Test program already compiled elsewhere, just copy it to this test
    add_custom_target(${TEST_TARGET} COMMAND ${CMAKE_COMMAND} -E copy
      "${REUSED_TEST_PROGRAM_PATH}" "${TARGET_TEST_PROGRAM_PATH}"
      DEPENDS "${REUSED_TEST_PROGRAM_PATH}")

    # Copy the test program when building test programs
    add_dependencies(build-tests ${TEST_TARGET})
  endif (${ARGC} LESS 2)
endmacro(PREPARE_TEST_PROGRAM TEST PROGRAM)

#
# Schedules a test which uses the ANaConDA framework.
#
# ADD_ANACONDA_TEST(<test>)
#
macro(ADD_ANACONDA_TEST TEST)
  # Clear variables set by the previous calls to the macro
  unset(TEST_CONFIG_ANALYSER)
  unset(TEST_CONFIG_PROGRAM)
  unset(TEST_CONFIG_FILTER)
  unset(TEST_CONFIG_CFLAGS)
  unset(TEST_CONFIG_LDFLAGS)
  unset(TEST_CONFIG_TIMEOUT)

  # Load the test configuration
  LOAD_TEST_CONFIG(${TEST})

  # Some analyser is needed to perform the test
  if (NOT TEST_CONFIG_ANALYSER)
    message(FATAL_ERROR "Test ${TEST}: no analyser specified")
  endif (NOT TEST_CONFIG_ANALYSER)

  # Configure the framework using its default settings
  CONFIGURE_TEST(${TEST} "$ENV{SOURCE_DIR}/framework/conf")
  # Update the default settings with the test settings
  CONFIGURE_TEST(${TEST} "${TEST_DIR}/${TEST}/conf")

  # Prepares the program needed for the test
  PREPARE_TEST_PROGRAM(${TEST} ${TEST_CONFIG_PROGRAM})

  # Get the name of the test (and test program)
  get_filename_component(TEST_NAME ${TEST} NAME)

  # Construct a command which performs the test
  set(CMD "$ENV{SOURCE_DIR}/tools/run.sh")

  # Configure the analyser using the test's settings
  set(CMD "${CMD} --config ${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/conf")

  # Specify the analyser and program used for the test
  set(CMD "${CMD} ${TEST_CONFIG_ANALYSER}")
  set(CMD "${CMD} ${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.test${TEST_PROGRAM_EXT}")

  # Use an output filter if specified in the test configuration
  if (TEST_CONFIG_FILTER)
    # Escape all backslashes, double-quotes are escaped automatically
    string(REPLACE "\\" "\\\\" OUTPUT_FILTER ${TEST_CONFIG_FILTER})
    # Redirect the output of the test to the output filter 
    set(CMD "${CMD} | ${OUTPUT_FILTER}")
  endif (TEST_CONFIG_FILTER)

  # Redirect the output of the test to a file
  set(CMD "${CMD} &>${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.out")

  # Compare the output of the test with the expected result
  set(CMD "${CMD} && ${CMAKE_COMMAND} -E compare_files")
  set(CMD "${CMD} ${TEST_DIR}/${TEST}/${TEST_NAME}.result")
  set(CMD "${CMD} ${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.out")

  # Schedule the test to perform
  add_test(${TEST} bash -o pipefail -c "${CMD}")

  # Set a timeout for the test
  if (TEST_CONFIG_TIMEOUT)
    set_tests_properties(${TEST} PROPERTIES TIMEOUT ${TEST_CONFIG_TIMEOUT})
  endif (TEST_CONFIG_TIMEOUT)
endmacro(ADD_ANACONDA_TEST)

#
# Schedules all tests in a specific folder. All of these tests should use the
#   ANaConDA framework.
#
# ADD_ANACONDA_TESTS(<folder>)
#
macro(ADD_ANACONDA_TESTS FOLDER)
  file(GLOB DIRECTORIES RELATIVE ${TEST_DIR} ${TEST_DIR}/${FOLDER}/*)

  # Find all directories containing tests to perform
  foreach (DIRECTORY ${DIRECTORIES})
    if (IS_DIRECTORY ${TEST_DIR}/${DIRECTORY})
      ADD_ANACONDA_TEST(${DIRECTORY})
    endif (IS_DIRECTORY ${TEST_DIR}/${DIRECTORY})
  endforeach (DIRECTORY ${DIRECTORIES})
endmacro(ADD_ANACONDA_TESTS)

# End of file Tests.cmake
