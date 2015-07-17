/**
 * @brief Contains implementation of functions for managing DLLs.
 *
 * A file containing implementation of functions for managing DLLs.
 *
 * @file      dll.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-17
 * @date      Last Update 2015-07-17
 * @version   0.1
 */

#include "dll.h"

// Microsoft linker provides the address of the DOS header in this variable
extern "C" IMAGE_DOS_HEADER __ImageBase;
// The address of the DOS header is also a handle of the module
#define CURRENT_MODULE_HANDLE ((HINSTANCE)&__ImageBase)

/**
 * Gets a handle of the ANaConDA framework.
 *
 * @return A handle of the ANaConDA framework.
 */
HMODULE getAnacondaFrameworkHandle()
{
  return CURRENT_MODULE_HANDLE;
}

/** End of file dll.cpp **/
