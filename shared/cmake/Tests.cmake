#
# A CMake module which automates testing.
#
# File:      Tests.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2016-03-24
# Date:      Last Update 2016-06-03
# Version:   0.8.1
#

# Enable commands for defining tests 
enable_testing()

# A target which compiles test programs
add_custom_target(build-tests)

# A directory used to perform the tests
set(TEST_WORK_DIR test)

#
# Loads a test configuration.
#
# LOAD_TEST_CONFIG(<test>)
#
macro(LOAD_TEST_CONFIG TEST)
  # Get the name of the configuration file (without the .conf extension)
  get_filename_component(TEST_CONFIG_NAME ${TEST} NAME)

  # Load the test configuration, ignore invalid entries
  file(STRINGS "${TEST_DIR}/${TEST}/${TEST_CONFIG_NAME}.conf" CONFIG_ENTRIES
    REGEX "[^=]*=.*")

  foreach (CONFIG_ENTRY ${CONFIG_ENTRIES})
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

  # Store the test program's executable in the test folder
  set_target_properties(${TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}"
    RUNTIME_OUTPUT_NAME "${TEST_PROGRAM_NAME}.test"
    COMPILE_FLAGS "-g -O0")

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

    # Test program already compiled elsewhere, just copy it to this test
    add_custom_target(${TEST_TARGET} COMMAND ${CMAKE_COMMAND} -E copy
      "${TEST_DIR}/${ARGV1}/${TEST_WORK_DIR}/${TEST_PROGRAM_NAME}.test"
      "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.test" DEPENDS
      "${TEST_DIR}/${ARGV1}/${TEST_WORK_DIR}/${TEST_PROGRAM_NAME}.test")

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

  # Load the test configuration
  LOAD_TEST_CONFIG(${TEST})

  # Some analyser is needed to perform the test
  if (NOT TEST_CONFIG_ANALYSER)
    message(FATAL_ERROR "Test ${TEST}: no analyser specified")
  endif (NOT TEST_CONFIG_ANALYSER)

  # Configure the framework using its default settings
  file(COPY "$ENV{SOURCE_DIR}/framework/conf"
    DESTINATION "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}")
  # Update the default settings with the test settings
  file(COPY "${TEST_DIR}/${TEST}/conf"
    DESTINATION "${TEST_DIR}/${TEST}/${TEST_WORK_DIR}")

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
  set(CMD "${CMD} ${TEST_DIR}/${TEST}/${TEST_WORK_DIR}/${TEST_NAME}.test")

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
