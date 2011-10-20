/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2011-10-20
 * @version   0.1.0.1
 */

#include "pin.H"

#include "settings.h"

/**
 * Instruments an image (executable, shared object, dynamic library, ...).
 *
 * @param img An object representing the image.
 * @param v A pointer to arbitrary data.
 */
VOID image(IMG img, VOID *v)
{
  //
}

/**
 * Instruments and runs a program to be analysed. Also initialises the PIN
 *   dynamic instrumentation framework.
 *
 * @param argc A number of arguments passed to the PIN run script.
 * @param argv A list of arguments passed to the PIN run script.
 * @return @em 0 if the program was executed successfully.
 */
int main(int argc, char *argv[])
{
  // An object containing the ANaConDA framework settings
  Settings settings;

  // Load the ANaConDA framework settings
  loadSettings(settings);

#ifdef DEBUG
  // Print ANaConDA framework settings
  printSettings(settings);
#endif

  // Needed for retrieving info about source file and line and column numbers
  PIN_InitSymbols();

  // Initialise the PIN dynamic instrumentation framework
  PIN_Init(argc, argv);

  // Instrument the program to be analysed
  IMG_AddInstrumentFunction(image, 0);

  // Run the instrumented version of the program to be analysed
  PIN_StartProgram();

  // Program finished its run, no post-execution tasks needed here for now
  return 0;
}

/** End of file anaconda.cpp **/
