/**
 * @brief A file containing definitions of classes and functions for accessing
 *   DWARF debugging information in PIN.
 *
 * A file containing definitions of classes and functions for accessing DWARF
 *   debugging information in PIN.
 *
 * @file      pin_dw_die.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-12
 * @date      Last Update 2011-10-12
 * @version   0.1
 */

#ifndef __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__
  #define __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__

#include "pin.H"

void dwarf_open(IMG image);

bool dwarf_get_variable(ADDRINT rtnAddr, ADDRINT insnAddr, ADDRINT accessAddr,
  INT32 size, const CONTEXT *registers, std::string& name, std::string& type,
  UINT32 *offset = NULL);

#endif /* __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__ */

/** End of file pin_dw_die.h **/
