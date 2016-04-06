#
# A CMake module which automates testing.
#
# File:      Tests.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2016-03-24
# Date:      Last Update 2016-04-06
# Version:   0.3
#

# Enable commands for defining tests 
enable_testing()

# A target which compiles test programs
add_custom_target(build-tests)

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
  # Collect source files of the program needed for the test
  aux_source_directory(${TEST_DIR}/${TEST} TEST_SOURCES)

  # Do not compile the test program when building the main program
  add_executable(${TEST} EXCLUDE_FROM_ALL ${TEST_SOURCES})

  # The name of the test may be in the <directory>/<test> format
  get_filename_component(TEST_PROGRAM_DIR ${TEST} DIRECTORY)
  get_filename_component(TEST_PROGRAM_NAME ${TEST} NAME)

  # Store the test program's executable in the test folder
  set_target_properties(${TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}/${TEST_PROGRAM_DIR}"
    RUNTIME_OUTPUT_NAME "${TEST_PROGRAM_NAME}.test")

  # Compile the test program when building test programs 
  add_dependencies(build-tests ${TEST})
endmacro(COMPILE_TEST_PROGRAM)

#
# Schedules a test which uses the ANaConDA framework.
#
# ADD_ANACONDA_TEST(<test>)
#
macro(ADD_ANACONDA_TEST TEST)
  # Load the test configuration
  LOAD_TEST_CONFIG(${TEST})

  # Some analyser is needed to perform the test
  if (NOT TEST_CONFIG_ANALYSER)
    message(FATAL_ERROR "Test ${TEST}: no analyser specified")
  endif (NOT TEST_CONFIG_ANALYSER)

  # Compile the program needed for the test
  COMPILE_TEST_PROGRAM(${TEST})

  # Construct a command which performs the test
  set(CMD "$ENV{ANACONDA_FRAMEWORK_HOME}/tools/run.sh")

  # Specify the analyser and program used for the test
  set(CMD ${CMD} "${TEST_CONFIG_ANALYSER}")
  set(CMD ${CMD} "${TEST_DIR}/${TEST}.test")

  # Schedule the test to perform
  add_test(${TEST} ${CMD})
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
