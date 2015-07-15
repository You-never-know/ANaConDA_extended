/**
 * @brief Contains implementation of functions for managing Windows PE files.
 *
 * A file containing implementation of functions for managing Windows Portable
 *   Executable (PE) files.
 *
 * @file      pe.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-15
 * @date      Last Update 2015-07-15
 * @version   0.1
 */

#include "pe.h"

#include <iomanip>
#include <iostream>

// Computes the actual address of a data given as offset from the image base
#define RVA2ADDRESS(base, offset) ((BYTE*)base + offset)
// Returns true if both addresses are the same (are equal)
#define SAMEADDRESS(addr1, addr2) ((BYTE*)addr1 == (BYTE*)addr2)

/**
 * Gets an address of a specific data directory of a Windows PE file.
 *
 * @tparam T A type of the data directory. If not specified, @c BYTE type will
 *   be used and @c BYTE pointer will be returned by the function. This allows
 *   correct computations when using the pointer as an address.
 *
 * @param module A handle identifying the Windows PE file.
 * @param type A type of the data directory (see the IMAGE_DIRECTORY_ENTRY_*
 *   constants).
 * @return An address of a specific data directory of a Windows PE file.
 */
template < typename T = BYTE >
T* getDataDir(HMODULE module, USHORT type)
{
  // Module handle is also its base address, at this address is the DOS header
  IMAGE_DOS_HEADER* iDosHeader = (IMAGE_DOS_HEADER*)module;

  // The e_lfanew attribute contains the RVA (relative virtual address) of the
  // Windows PE header, RVA is relative to the module base address (its handle)
  IMAGE_NT_HEADERS* iNtHeader = (IMAGE_NT_HEADERS*)RVA2ADDRESS(module,
    iDosHeader->e_lfanew);

  if (!SAMEADDRESS(module, iNtHeader->OptionalHeader.ImageBase))
  { // Invalid handle
    return NULL;
  }

  // Return pointer to the chosen data directory
  return (T*)RVA2ADDRESS(module,
    iNtHeader->OptionalHeader.DataDirectory[type].VirtualAddress);
}

/**
 * Gets an export table of a Windows PE file.
 *
 * @param module A handle identifying the Windows PE file.
 * @return An export table of the specified Windows PE file.
 */
ExportTable* getExportTable(HMODULE module)
{
  // Get the address of the module export table
  IMAGE_EXPORT_DIRECTORY* iExportDir = getDataDir< IMAGE_EXPORT_DIRECTORY >(
    module, IMAGE_DIRECTORY_ENTRY_EXPORT);

  if (iExportDir == NULL) return NULL; // Invalid handle

  // Helper variables
  ExportTable* eTab = new ExportTable();

  // An array containing RVAs pointing to the addresses of exported functions
  DWORD* addresses = (DWORD*)RVA2ADDRESS(module, iExportDir->AddressOfFunctions);

  for (DWORD i = 0; i < iExportDir->NumberOfFunctions; ++i)
  { // Extract the actual addresses of the exported functions
    eTab->functions.push_back(ExportedFunction(iExportDir->Base + i,
      RVA2ADDRESS(module, addresses[i])));
  }

  // An array containing RVAs pointing to the names of exported functions
  DWORD* names = (DWORD*)RVA2ADDRESS(module, iExportDir->AddressOfNames);
  // An array containing ordinals of functions associated with the names in the
  // names array above, the (name, ordinal) pair always share the same index in
  // this array and the names array allowing to associate the name with address
  WORD* ordinals = (WORD*)RVA2ADDRESS(module, iExportDir->AddressOfNameOrdinals);

  for (DWORD i = 0; i < iExportDir->NumberOfNames; ++i)
  { // Assign names to all functions which were exported by name
    eTab->functions.at(ordinals[i]).name = (LPSTR)RVA2ADDRESS(module, names[i]);
  }

  return eTab;
}

/**
 * Prints the export table to the standard output.
 *
 * @param table A table containing information about exported functions.
 */
void printExportTable(ExportTable* table)
{
  for (unsigned int i = 0; i < table->functions.size(); ++i)
  { // Print all exported functions
    ExportedFunction& function = table->functions[i];

    std::cout << "[" << std::setw(4) << std::dec << function.ordinal << "] 0x"
      << std::hex << (void*)function.address  << std::dec << " -> "
      << ((function.name) ? function.name : "<none>") << "\n";
  }
}

/** End of file pe.cpp **/
