/**
 * @brief A file containing definitions of classes and functions for accessing
 *   debugging information in PIN.
 *
 * A file containing definitions of classes and functions for accessing
 *   debugging information in PIN.
 *
 * @file      pin_die.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-13
 * @date      Last Update 2012-02-11
 * @version   0.1.1
 */

#ifndef __LIBPIN_DIE__PIN_DIE_H__
  #define __LIBPIN_DIE__PIN_DIE_H__

#include <stdlib.h>

#include "pin.H"

void DIE_Open(IMG image);

void DIE_Print(IMG image);

bool DIE_GetVariable(ADDRINT rtnAddr, ADDRINT insnAddr, ADDRINT accessAddr,
  INT32 size, const CONTEXT *registers, std::string& name, std::string& type,
  UINT32 *offset = NULL);

#endif /* __LIBPIN_DIE__PIN_DIE_H__ */

/** End of file pin_die.h **/
