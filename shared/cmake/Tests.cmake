#
# A CMake module which automates testing.
#
# File:      Tests.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2016-03-24
# Date:      Last Update 2016-04-05
# Version:   0.2
#

# Enable commands for defining tests 
enable_testing()

# A target which compiles test programs
add_custom_target(build-tests)

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
  # Compile the program needed for the test
  COMPILE_TEST_PROGRAM(${TEST})

  # Construct a command which performs the test
  set(CMD "$ENV{ANACONDA_FRAMEWORK_HOME}/tools/run.sh")

  # Specify the analyser and program used for the test 
  set(CMD ${CMD} "event-printer")
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
