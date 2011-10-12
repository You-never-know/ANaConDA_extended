/**
 * @brief A file containing implementation of classes and functions for
 *   accessing debugging information in PIN.
 *
 * A file containing implementation of classes and functions for accessing
 *   debugging information in PIN.
 *
 * @file      pin_die.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-13
 * @date      Last Update 2011-10-12
 * @version   0.1
 */

#include "pin_die.h"

#include "dwarf/pin_dw_die.h"

/**
 * Opens an image (executable, shared object, dynamic library, ...).
 *
 * @param image An object representing the image.
 */
void DIE_Open(IMG image)
{
  dwarf_open(image);
}

/**
 * Gets a variable stored on an accessed address.
 *
 * @param rtnAddr An address of the routine in which the variable was accessed.
 * @param insnAddr An address of the instruction which accessed the variable.
 * @param accessAddr The accessed address.
 * @param size A number of bytes accessed.
 * @param registers A structure containing register values.
 * @param name A reference to a string to which will be stored the name of the
 *   variable.
 * @param type A reference to a string to which will be stored the type of the
 *   variable.
 * @param offset A pointer to an integer to which will be stored the offset if
 *   only part of the variable was accessed.
 * @return @em True if the variable was found, @em false otherwise.
 */
bool DIE_GetVariable(ADDRINT rtnAddr, ADDRINT insnAddr, ADDRINT accessAddr,
  INT32 size, const CONTEXT *registers, std::string& name, std::string& type,
  UINT32 *offset)
{
  return dwarf_get_variable(rtnAddr, insnAddr, accessAddr, size, registers,
    name, type, offset);
}

/** End of file pin_die.cpp **/
