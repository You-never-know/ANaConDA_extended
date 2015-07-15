/**
 * @brief Contains definitions of functions for managing Windows PE files.
 *
 * A file containing definitions of functions for managing Windows Portable
 *   Executable (PE) files.
 *
 * @file      pe.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-15
 * @date      Last Update 2015-07-15
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H_
  #define __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H_

#include <windows.h>

#include <vector>

/**
 * @brief A structure containing information about an exported function.
 */
typedef struct ExportedFunction_s
{
  DWORD ordinal; //!< A number identifying the exported function.
  LPSTR name; //!< A name of the exported function. May be @c NULL.
  BYTE* address; //!< The address of the exported function.

  /**
   * Constructs an object describing an exported function.
   *
   * @param o A number identifying the exported function.
   * @param a The address of the exported function.
   */
  ExportedFunction_s(DWORD o, BYTE* a) : ordinal(o), address(a), name(NULL) {}
} ExportedFunction;

/**
 * @brief A structure containing information about all exported functions.
 */
typedef struct ExportTable_s
{
  std::vector< ExportedFunction > functions; //!< A list of exported functions.
} ExportTable;

ExportTable* getExportTable(HMODULE module);
void printExportTable(ExportTable* table);

#endif /* __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H_ */

/** End of file pe.h **/
