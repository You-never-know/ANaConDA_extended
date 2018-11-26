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
 * @brief A file containing definitions of classes and functions for accessing
 *   DWARF debugging information in PIN.
 *
 * A file containing definitions of classes and functions for accessing DWARF
 *   debugging information in PIN.
 *
 * @file      pin_dw_die.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-12
 * @date      Last Update 2012-02-11
 * @version   0.1.1
 */

#ifndef __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__
  #define __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__

#include <stdlib.h>

#include "pin.H"

void dwarf_open(IMG image);

void dwarf_print(IMG image);

bool dwarf_get_variable(ADDRINT rtnAddr, ADDRINT insnAddr, ADDRINT accessAddr,
  INT32 size, const CONTEXT *registers, std::string& name, std::string& type,
  UINT32 *offset = NULL);

#endif /* __LIBPIN_DIE__DWARF__PIN_DW_DIE_H__ */

/** End of file pin_dw_die.h **/
