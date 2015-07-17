/**
 * @brief Contains implementation of functions for managing DLLs.
 *
 * A file containing implementation of functions for managing DLLs.
 *
 * @file      dll.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-17
 * @date      Last Update 2015-07-17
 * @version   0.2
 */

#include "dll.h"

// Microsoft linker provides the address of the DOS header in this variable
extern "C" IMAGE_DOS_HEADER __ImageBase;
// The address of the DOS header is also a handle of the module
#define CURRENT_MODULE_HANDLE ((HINSTANCE)&__ImageBase)

/**
 * Gets a handle of a module which contains a specific address.
 *
 * @param address An address belonging to the module.
 * @return A handle of the module to which the specified address belongs.
 */
HMODULE getModuleHandleByAddress(void* address)
{
  // Helper variables
  MEMORY_BASIC_INFORMATION mbi;

  // Get information about the memory of the module containing the address
  if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0) return 0;

  // Allocation base is the base address of the module and also its handle
  return reinterpret_cast< HMODULE >(mbi.AllocationBase);
}

/**
 * Gets a handle of the hidden ANaConDA framework.
 *
 * @return A handle of the hidden ANaConDA framework.
 */
HMODULE getHiddenAnacondaFrameworkHandle()
{
  return CURRENT_MODULE_HANDLE;
}

/**
 * Gets a handle of the hidden PIN framework.
 *
 * @return A handle of the hidden PIN framework.
 */
HMODULE getHiddenPinFrameworkHandle()
{
  // This will get us the address of the GetProcAddress functions from the PIN
  // framework instead of the kernel32.dll library as PIN replaces it with its
  // own version so we will get the address of the PIN's version here
  return getModuleHandleByAddress(&GetProcAddress);
}

/** End of file dll.cpp **/
