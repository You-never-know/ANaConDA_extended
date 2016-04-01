#
# A CMake module which automates testing.
#
# File:      Tests.cmake
# Author:    Jan Fiedor (fiedorjan@centrum.cz)
# Date:      Created 2016-03-24
# Date:      Last Update 2016-04-01
# Version:   0.1
#

# Enable commands for defining tests 
enable_testing()

#
# Schedules a test which uses the ANaConDA framework.
#
# ADD_ANACONDA_TEST(<test>)
#
macro(ADD_ANACONDA_TEST TEST)
  # Construct a command which performs the test
  set(CMD "$ENV{ANACONDA_FRAMEWORK_HOME}/tools/run.sh" "--help")

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
