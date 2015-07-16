/**
 * @brief Contains definitions of functions for managing Windows PE files.
 *
 * A file containing definitions of functions for managing Windows Portable
 *   Executable (PE) files.
 *
 * @file      pe.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-15
 * @date      Last Update 2015-07-16
 * @version   0.3
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H__
  #define __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H__

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
  LPSTR module; //!< A name of the module to which the export table belongs.
  std::vector< ExportedFunction > functions; //!< A list of exported functions.
} ExportTable;

/**
 * @brief A structure containing information about an imported function.
 */
typedef struct ImportedFunction_s
{
  DWORD ordinal; //!< A number identifying the imported function.
  LPSTR name; //!< A name of the imported function. May be @c NULL.
  BYTE** address; //!< A pointer to the address of the imported function.

  /**
   * Constructs an object describing an imported function.
   */
  ImportedFunction_s() : ordinal(0), name(NULL), address(NULL) {}
} ImportedFunction;

/**
 * @brief A structure containing information about all functions imported from
 *   a specific module.
 */
typedef struct ModuleTable_s
{
  LPSTR name; //!< A name of the module.
  /**
   * @brief A list of functions imported by the module.
   */
  std::vector< ImportedFunction > functions;

  /**
   * Constructs an object containing information about all functions imported
   *   from a specific module.
   *
   * @param n A name of the module.
   */
  ModuleTable_s(LPSTR n) : name(n) {}
} ModuleTable;

/**
 * @brief A structure containing information about all imported functions.
 */
typedef struct ImportTable_s
{
  /**
   * @brief A list of all modules from which are the functions imported.
   */
  std::vector< ModuleTable > modules;
} ImportTable;

ExportTable* getExportTable(HMODULE module);
void printExportTable(ExportTable* table);

ImportTable* getImportTable(HMODULE module);
void printImportTable(ImportTable* table);

bool redirectCalls(HMODULE from, HMODULE to);

#endif /* __ANACONDA_FRAMEWORK__UTILS__WINDOWS__PE_H__ */

/** End of file pe.h **/
