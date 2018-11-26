/*
 * Copyright (C) 2011-2018 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

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
 * @date      Last Update 2012-05-31
 * @version   0.1.2
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
#ifdef TARGET_LINUX
  dwarf_open(image);
#endif
}

/**
 * Prints debugging information present in an image (executable, shared object,
 *   dynamic library, ...).
 *
 * @param image An object representing the image.
 */
void DIE_Print(IMG image)
{
#ifdef TARGET_LINUX
  dwarf_print(image);
#endif
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
#ifdef TARGET_LINUX
  return dwarf_get_variable(rtnAddr, insnAddr, accessAddr, size, registers,
    name, type, offset);
#else
  return false;
#endif
}

/** End of file pin_die.cpp **/
