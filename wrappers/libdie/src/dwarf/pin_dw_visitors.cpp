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
 * @brief A file containing implementation of classes for visiting DWARF
 *   entries.
 *
 * A file containing implementation of classes for visiting various DWARF
 *   entries.
 *
 * @file      pin_dw_visitors.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-15
 * @date      Last Update 2013-03-08
 * @version   0.2
 */

#include "pin_dw_visitors.h"

/**
 * Constructs a DwFunctionIndexer object.
 */
DwFunctionIndexer::DwFunctionIndexer(Dwarf_Function_Map& index) : m_index(index)
{
}

/**
 * Destroys a DwFunctionIndexer object.
 */
DwFunctionIndexer::~DwFunctionIndexer()
{
}

/**
 * Visits a DWARF subprogram debugging information entry object.
 *
 * @param s A DWARF subprogram debugging information entry object.
 */
void DwFunctionIndexer::visit(DwSubprogram& s)
{
  m_index[s.getLowPC()] = &s;
}

/**
 * Constructs a DwGlobalVariableIndexer object.
 */
DwGlobalVariableIndexer::DwGlobalVariableIndexer(Dwarf_Variable_Map& index)
  : m_index(index)
{
}

/**
 * Destroys a DwGlobalVariableIndexer object.
 */
DwGlobalVariableIndexer::~DwGlobalVariableIndexer()
{
}

/**
 * Visits a DWARF variable debugging information entry object.
 *
 * @param v A DWARF variable debugging information entry object.
 */
void DwGlobalVariableIndexer::visit(DwVariable& v)
{
  if (v.isGlobal())
  { // Index the whole address range at which is the variable situated
    m_index.insert(v.getLocation()->lr_number, v.getLocation()->lr_number
      + v.getSize(), &v);
  }
}

/** End of file pin_dw_visitors.cpp **/
