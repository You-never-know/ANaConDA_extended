/**
 * @brief A file containing the entry part of the ANaConDA framework.
 *
 * A file containing the entry part of the ANaConDA framework.
 *
 * @file      anaconda.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-17
 * @date      Last Update 2011-10-27
 * @version   0.1.2
 */

#include "pin.H"

#include "pin_die.h"

#include "settings.h"

/**
 * Instruments an image (executable, shared object, dynamic library, ...).
 *
 * @param img An object representing the image.
 * @param v A pointer to arbitrary data.
 */
VOID image(IMG img, VOID *v)
{
  // The pointer 'v' is a pointer to an object containing framework settings
  Settings *settings = static_cast< Settings* >(v);

  if (settings->isExcludedFromInstrumentation(img))
  { // The image should not be instrumented, log it for pattern debugging
    LOG("Image '" + IMG_Name(img) + "' will not be instrumented.\n");
    LOG("Debugging information in image '" + IMG_Name(img)
      + "' will not be extracted.\n");

    return;
  }

  // The image should be instrumented
  LOG("Instrumenting image '" + IMG_Name(img) + "'.\n");

  if (settings->isExcludedFromDebugInfoExtraction(img))
  { // Debugging information should not be extracted from the image
    LOG("Debugging information in image '" + IMG_Name(img)
      + "' will not be extracted.\n");
  }
  else
  { // Debugging information should be extracted from the image
    LOG("Extracting debugging information from image '" + IMG_Name(img)
      + "'.\n");

    // Open the image and extract debugging information from it
    DIE_Open(img);
  }
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
  Settings *settings = new Settings();

  // Load the ANaConDA framework settings
  settings->load();

#ifdef DEBUG
  // Print ANaConDA framework settings
  settings->print();
#endif

  // Needed for retrieving info about source file and line and column numbers
  PIN_InitSymbols();

  // Initialise the PIN dynamic instrumentation framework
  PIN_Init(argc, argv);

  // Instrument the program to be analysed
  IMG_AddInstrumentFunction(image, static_cast< VOID* >(settings));

  // Run the instrumented version of the program to be analysed
  PIN_StartProgram();

  // Free all the previously allocated memory
  delete settings;

  // Program finished its run, no post-execution tasks needed here for now
  return 0;
}

/** End of file anaconda.cpp **/
