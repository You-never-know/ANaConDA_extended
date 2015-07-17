/**
 * @brief Contains definitions of functions for managing DLLs.
 *
 * A file containing definitions of functions for managing DLLs.
 *
 * @file      dll.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-17
 * @date      Last Update 2015-07-17
 * @version   0.2
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__
  #define __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__

#include <windows.h>

HMODULE getModuleHandleByAddress(void* address);
HMODULE getHiddenAnacondaFrameworkHandle();
HMODULE getHiddenPinFrameworkHandle();

#endif /* __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__ */

/** End of file dll.h **/
