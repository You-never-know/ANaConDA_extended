/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of libdie.
 *
 * libdie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libdie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdie. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing implementation of classes for extracting, holding
 *   and manipulating the DWARF debugging information.
 *
 * A file containing implementation of classes for extracting, holding and
 *   manipulating the DWARF debugging information.
 *
 * @file      dw_die.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2012-01-20
 * @version   0.1.4.2
 */

#include "dw_die.h"

#include <assert.h>
#include <fcntl.h>

#include <iostream>
#include <stack>

#include "dw_visitors.h"

/**
 * Prints information about an error which occurred during a libdwarf operation
 *   and exits.
 *
 * @param err A structure containing information about the error.
 * @param ptr An arbitrary argument passed to the handler function.
 */
void dwarf_error_handler(Dwarf_Error err, Dwarf_Ptr ptr)
{
  std::cerr << "error: libdwarf: " << dwarf_errmsg(err) << " [error code "
    << dwarf_errno(err) << "]" << std::endl;
  exit(1);
}

/**
 * Accepts a DWARF debugging information entry visitor.
 *
 * @param visitor A DWARF debugging information entry visitor.
 */
void DwarfDebugInfo::accept(DwDieVisitor& visitor)
{
  // Helper variables
  std::list< boost::shared_ptr< DwCompileUnit > >::iterator it;

  for (it = m_compileUnitList.begin(); it != m_compileUnitList.end(); it++)
  { // Visit all contained DWARF compilation units
    it->get()->accept(visitor);
  }
}

/**
 * Accepts a DWARF debugging information entry tree traverser.
 *
 * @param traverser A DWARF debugging information entry tree traverser.
 */
void DwarfDebugInfo::accept(DwDieTreeTraverser& traverser)
{
  // Helper variables
  std::list< boost::shared_ptr< DwCompileUnit > >::iterator it;

  for (it = m_compileUnitList.begin(); it != m_compileUnitList.end(); it++)
  { // Visit all contained DWARF compilation units
    it->get()->accept(traverser);
  }
}

/**
 * Prints the DWARF debugging information to a standard output.
 */
void DwarfDebugInfo::printDebugInfo()
{
  // Helper variables
  DwDebugInfoPrinter printer;
  std::list< boost::shared_ptr< DwCompileUnit > >::iterator it;

  for (it = m_compileUnitList.begin(); it != m_compileUnitList.end(); it++)
  { // Print debugging information in each of the contained compile units
    it->get()->accept(printer);
  }
}

/**
 * Prints information about variables to a standard output.
 */
void DwarfDebugInfo::printVariables()
{
  // Helper variables
  DwVariablePrinter printer;
  std::list< boost::shared_ptr< DwCompileUnit > >::iterator it;

  for (it = m_compileUnitList.begin(); it != m_compileUnitList.end(); it++)
  { // Print information about variables in each of the contained compile units
    it->get()->accept(printer);
  }
}

// Initialise the current singleton instance
boost::shared_ptr< DwarfDebugInfoExtractor >
  DwarfDebugInfoExtractor::ms_instance(new DwarfDebugInfoExtractor());

/**
 * Constructs a DwarfDebugInfoExtractor object.
 */
DwarfDebugInfoExtractor::DwarfDebugInfoExtractor()
{
}

/**
 * Destroys a DwarfDebugInfoExtractor object.
 */
DwarfDebugInfoExtractor::~DwarfDebugInfoExtractor()
{
}

/**
 * Gets a singleton instance.
 *
 * @return The current singleton instance.
 */
DwarfDebugInfoExtractor *DwarfDebugInfoExtractor::Get()
{
  return ms_instance.get();
}

/**
 * Gets the DWARF debugging information from a file.
 *
 * @param filename A name of the file.
 * @return An object containing DWARF debug information.
 */
DwarfDebugInfo *DwarfDebugInfoExtractor::getDebugInfo(std::string filename)
{
  if (m_dbgInfos.find(filename) == m_dbgInfos.end())
  { // No debugging information extracted yet, extract it now
    m_dbgInfos[filename].reset(this->extractDebugInfo(filename));
  }

  return m_dbgInfos[filename].get();
}

/**
 * Extracts the DWARF debugging information from a file.
 *
 * @param filename A name of the file.
 * @return An object containing DWARF debugging information.
 */
DwarfDebugInfo *DwarfDebugInfoExtractor::extractDebugInfo(std::string filename)
{
  // Helper variables
  int dwRes = 0;
  Dwarf_Error err;

  // Create an object to which will be the DWARF debugging information stored
  DwarfDebugInfo *dwDebugInfo = new DwarfDebugInfo();

  // Open the file containing the DWARF debugging information
  int fd = open(filename.c_str(), O_RDONLY);

  // Initialise the DWARF library, use the error parameter here, because the
  // supplied error handler is not active until the initialisation is complete
  dwRes = dwarf_init(fd, DW_DLC_READ, dwarf_error_handler, NULL,
    &dwDebugInfo->m_dbg, &err);

  if (dwRes == DW_DLV_NO_ENTRY)
  { // The specified file does not contain any DWARF debugging information
    throw ExtractionError("no DWARF debugging information found in '" + filename
      + "'");
  }
  else if (dwRes != DW_DLV_OK)
  { // An error occurred when accessing the DWARF debugging information
    throw ExtractionError("cannot access DWARF debugging information in '"
      + filename + "': " + dwarf_errmsg(err));
  }

  // Helper variables
  Dwarf_Unsigned cu_header_length = 0;
  Dwarf_Half version_stamp = 0;
  Dwarf_Unsigned abbrev_offset = 0;
  Dwarf_Half address_size = 0;
  Dwarf_Half offset_size = 0;
  Dwarf_Half extension_size = 0;
  Dwarf_Unsigned next_cu_header = 0;

  // Process all DWARF compile units (CUs) in the file
  while ((dwRes = dwarf_next_cu_header_b(dwDebugInfo->m_dbg, &cu_header_length,
    &version_stamp, &abbrev_offset, &address_size, &offset_size,
    &extension_size, &next_cu_header, NULL)) != DW_DLV_NO_ENTRY)
  { // Extract debugging information from a DWARF compile unit
    if (dwRes == DW_DLV_ERROR)
    { // An error occurred when accessing the next DWARF compile unit (CU)
      throw ExtractionError("cannot access DWARF debugging information stored "\
        "in a DWARF compile unit (CU).");
    }

    // Helper variables
    Dwarf_Die curr = 0;
    Dwarf_Die next = 0;

    // Process all DWARF debugging information entries (DIEs) in the current CU
    while ((dwRes = dwarf_siblingof(dwDebugInfo->m_dbg, curr, &next,
      NULL)) != DW_DLV_NO_ENTRY)
    { // Extract all DWARF DIEs in the current compile unit (CU)
      if (dwRes == DW_DLV_ERROR)
      { // An error occurred when accessing the next DWARF DIE in the current CU
        throw ExtractionError(
          "cannot access DWARF debug information entry (DIE).");
      }

      // Extract a DWARF DIE with all its child DIEs from the current DWARF CU
      DwDie* die = this->extractDebugInfoEntry(next, dwDebugInfo->m_dbg);

      // Add the DIE to the list of CUs extracted from the file
      dwDebugInfo->m_compileUnitList.push_back(
        boost::shared_ptr< DwCompileUnit >(
          dynamic_cast< DwCompileUnit* >(die)));

      // Move to the next DWARF DIE
      curr = next;
    }
  }

  // A visitor which replaces offsets with pointers to DwDie objects
  DwReferenceLinker referenceLinker;
  // A visitor which replaces source file indexes with pointers to their names
  DwSourceFileIndexEvaluator srcFileIndexEvaluator;

  // Replace offsets with pointers to DwDie objects
  dwDebugInfo->accept(referenceLinker);
  // Replace source file indexes with pointers to source file names
  dwDebugInfo->accept(srcFileIndexEvaluator);

  // Return the DWARF debugging information extracted from the file
  return dwDebugInfo;
}

/**
 * Extracts a DWARF debugging information entry with all debugging information
 *   entries contained in it (all its children).
 *
 * @param die A DWARF debugging information entry.
 * @param dbg A DWARF handle for accessing debugging records.
 * @return A DWARF debugging information entry object.
 */
DwDie* DwarfDebugInfoExtractor::extractDebugInfoEntry(Dwarf_Die& die,
  Dwarf_Debug& dbg)
{
  // Type definitions
  typedef std::pair< Dwarf_Die, DwDie* > DieWithParent;

  // Helper variables
  Dwarf_Half tag;
  Dwarf_Die next;
  DieWithParent curr;
  DwDie* root = NULL;
  DwDie* parent = NULL;
  std::stack< DieWithParent > dies;

  // Create a factory for creating DWARF debugging information entry objects
  DwDieFactory factory;

  // Get the type of the specified DWARF debugging information entry
  dwarf_tag(die, &tag, NULL);

  // Create a DWARF DIE object of the specified type and set it as a root DIE
  root = factory.createTag(tag, die);

  if (dwarf_child(die, &next, NULL) != DW_DLV_NO_ENTRY)
  { // The DWARF DIE have at least one children, schedule it for processing
    dies.push(DieWithParent(next, root));
  }

  // Process all child DIEs of the specified DIE
  while (!dies.empty())
  { // Get the DWARF DIE which should be processed (extracted)
    curr = dies.top();
    // Remove the DIE from the to-be-processed queue
    dies.pop();

    // Get the type of the currently processed DWARF DIE
    dwarf_tag(curr.first, &tag, NULL);

    // Create a DIE of the specified type and set it as a child of its parent
    parent = factory.createTag(tag, curr.first, curr.second);

#ifdef DEBUG
    if (parent == NULL)
    { // Print an error message if the DIE object could not be created
      const char *name = NULL;
      // Get the name of the tag which should the DIE object represent
      dwarf_get_TAG_name(tag, &name);
      // Print the name of the tag which should the DIE object represent
      std::cerr << "Could not create a DIE object for the '" << name << "' tag."
        << std::endl;
    }
#endif

    // The DIE object must be created successfully in order to continue
    assert(parent != NULL);

    if (dwarf_child(curr.first, &next, NULL) != DW_DLV_NO_ENTRY)
    { // The current DIE have some child DIEs, schedule the first child DIE of
      // the current DIE for processing, the current DIE will be its parent DIE
      dies.push(DieWithParent(next, parent));
    }

    if (dwarf_siblingof(dbg, curr.first, &next, NULL) != DW_DLV_NO_ENTRY)
    { // The current DIE have some sibling DIEs, schedule the next sibling DIE
      // of the current DIE for processing, the DIE which is the parent of the
      // current DIE will be the parent of the sibling DIE
      dies.push(DieWithParent(next, curr.second));
    }
  }

  // Return the root DIE (the DIE object representing the specified DWARF DIE)
  return root;
}

/** End of file dw_die.cpp **/
