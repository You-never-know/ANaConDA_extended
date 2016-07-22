/**
 * @brief Contains implementation of functions for managing Windows PE files.
 *
 * A file containing implementation of functions for managing Windows Portable
 *   Executable (PE) files.
 *
 * @file      pe.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-15
 * @date      Last Update 2016-07-22
 * @version   0.5.1
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
  if (module == NULL) return NULL; // Invalid handle

  // Get the address of the module export table
  IMAGE_EXPORT_DIRECTORY* iExportDir = getDataDir< IMAGE_EXPORT_DIRECTORY >(
    module, IMAGE_DIRECTORY_ENTRY_EXPORT);

  if (iExportDir == NULL) return NULL; // Invalid handle

  // Helper variables
  ExportTable* eTab = new ExportTable();

  // Get the name of the module
  eTab->module = (LPSTR)RVA2ADDRESS(module, iExportDir->Name);

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
  std::cout << "Module: " << table->module << "\n";

  for (unsigned int i = 0; i < table->functions.size(); ++i)
  { // Print all exported functions
    ExportedFunction& function = table->functions[i];

    std::cout << "[" << std::setw(4) << std::dec << function.ordinal << "] 0x"
      << std::hex << (void*)function.address  << std::dec << " -> "
      << ((function.name) ? function.name : "<none>") << "\n";
  }
}

/**
 * Gets an import table of a Windows PE file.
 *
 * @param module A handle identifying the Windows PE file.
 * @return An import table of the specified Windows PE file.
 */
ImportTable* getImportTable(HMODULE module)
{
  if (module == NULL) return NULL; // Invalid handle

  // Get the address of the module import table (array of import descriptors)
  IMAGE_IMPORT_DESCRIPTOR* iImportDesc = getDataDir< IMAGE_IMPORT_DESCRIPTOR >(
    module, IMAGE_DIRECTORY_ENTRY_IMPORT);

  if (iImportDesc == NULL) return NULL; // Invalid handle

  // Helper variables
  ImportTable* iTab = new ImportTable();

  while (iImportDesc->FirstThunk != NULL)
  { // Process all imported modules (each descriptor describes one module)
    iTab->modules.push_back(ModuleTable((LPSTR)RVA2ADDRESS(module,
      iImportDesc->Name)));

    // Helper variables
    ModuleTable& mTab = iTab->modules.back();

    // Array of information about each of the imported functions
    IMAGE_THUNK_DATA* origThunk = (IMAGE_THUNK_DATA*)RVA2ADDRESS(module,
      iImportDesc->OriginalFirstThunk);
    IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)RVA2ADDRESS(module,
      iImportDesc->FirstThunk);

    while (origThunk->u1.AddressOfData != NULL)
    { // Process all imported functions (each thunk describes one function)
      ImportedFunction iFunc;

      if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.AddressOfData))
      { // Function imported by ordinal (number)
        iFunc.ordinal = IMAGE_ORDINAL(origThunk->u1.AddressOfData);
      }
      else
      { // Function imported by name
        iFunc.name = ((IMAGE_IMPORT_BY_NAME*)RVA2ADDRESS(module,
          origThunk->u1.AddressOfData))->Name;
      }

      // This is the address of the imported function's address, i.e., the value
      // is the address called when calling the imported function
      iFunc.address = (BYTE**)&thunk->u1.Function;

      mTab.functions.push_back(iFunc); // Save the imported function

      ++origThunk; // Move to the next imported function
      ++thunk;
    }

    ++iImportDesc; // Move to the next module
  }

  return iTab;
}

/**
 * Prints the import table to the standard output.
 *
 * @param table A table containing information about imported functions.
 */
void printImportTable(ImportTable* table)
{
  for (unsigned int i = 0; i < table->modules.size(); ++i)
  { // Print all modules from which are the functions imported
    ModuleTable& mTab = table->modules[i];

    std::cout << "Module: " << mTab.name << "\n";

    for (unsigned int j = 0; j < mTab.functions.size(); ++j)
    { // Print all functions imported from this module
      ImportedFunction& function = mTab.functions[j];

      std::cout << "  [" << std::setw(4) << std::dec << function.ordinal << "] "
        << ((function.name) ? function.name : "<none>") << " pointing at 0x"
        << std::hex << (void*)(*function.address)  << std::dec << "\n";
    }
  }
}

/**
 * Rebinds an imported function with a specific exported function.
 *
 * @param iTabAddr An address at which is stored the address of the imported
 *   function called by the module.
 * @param eFuncAddr An address of an exported function which should be called
 *   when the imported function is called.
 * @return @em True if the rebinding was successful, @em false otherwise.
 */
bool rebindFunction(void* iTabAddr, void* eFuncAddr)
{
  // Helper variables
  DWORD origPageProtection;
  DWORD prevPageProtection;

  // The import table is read-only after the initial rebinding, allow writes
  if (!VirtualProtectEx(GetCurrentProcess(), iTabAddr, sizeof(void*),
    PAGE_EXECUTE_READWRITE, &origPageProtection)) return false;

  // Replace the currently referenced exported function with a new one, i.e.,
  // do *iTabAddr = eFuncAddr with the right address size of sizeof(void*)
  memcpy(iTabAddr, &eFuncAddr, sizeof(void*));

  // Restore the original protection of the import table
  if (!VirtualProtectEx(GetCurrentProcess(), iTabAddr, sizeof(void*),
    origPageProtection, &prevPageProtection)) return false;

  return true;
}

/**
 * Redirect calls of imported functions to functions exported by a specific
 *   module.
 *
 * @param from A module which calls the imported functions.
 * @param to A module which exports the functions called by the first module.
 * @return @em True if the redirection was successful, @em false otherwise.
 */
bool redirectCalls(HMODULE from, HMODULE to)
{
  if (from == NULL || to == NULL) return false;

  // Get the import table of the first module
  ImportTable* iTab = getImportTable(from);

  if (iTab == NULL) return false; // Stop if the import table is not valid

  // Get the export table of the second module
  ExportTable* eTab = getExportTable(to);

  if (eTab == NULL)
  { // Export table not valid
    delete iTab;
    return false;
  }

  for (unsigned int i = 0; i < iTab->modules.size(); ++i)
  { // Search the import table for the functions exported by the second module
    ModuleTable& mTab = iTab->modules[i];

    if (strcmp(mTab.name, eTab->module) == 0)
    { // Found module containing the functions exported by the second module
      for (unsigned int j = 0; j < mTab.functions.size(); ++j)
      { // Redirect all imported functions
        for (unsigned int k = 0; k < eTab->functions.size(); ++k)
        { // Search for a matching exported function in the export table
          if (strcmp(mTab.functions[j].name, eTab->functions[k].name) == 0)
          { // Matching function found, correct the references
            rebindFunction(mTab.functions[j].address, eTab->functions[k].address);
          }
        }
      }
    }
  }

  // Free the import and export tables
  delete iTab;
  delete eTab;

  return true;
}

/** End of file pe.cpp **/
