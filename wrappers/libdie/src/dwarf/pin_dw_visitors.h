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
 * @brief A file containing definitions of classes for visiting DWARF entries.
 *
 * A file containing definitions of classes for visiting various DWARF entries.
 *
 * @file      pin_dw_visitors.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-15
 * @date      Last Update 2019-02-05
 * @version   0.2.1
 */

#ifndef __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__
  #define __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__

#include <map>

#include "libdie/dwarf/dw_classes.h"
#include "libdie/dwarf/dw_visitors.h"

#include "../util/ivalmap.hpp"

// Type definitions
typedef std::map< Dwarf_Addr, DwSubprogram* > Dwarf_Function_Map;
typedef IntervalMap< Dwarf_Addr, DwVariable* > Dwarf_Variable_Map;

/**
 * @brief A visitor for indexing functions.
 *
 * Indexes functions, i.e., creates a map mapping addresses of functions to
 *   DWARF subprogram debugging information entry objects.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-15
 * @date      Last Update 2011-09-15
 * @version   0.1
 */
class DwFunctionIndexer : public DwDieVisitor
{
  private: // User-defined variables
    Dwarf_Function_Map& m_index; //!< A table mapping addresses to functions.
  public: // Constructors
    DwFunctionIndexer(Dwarf_Function_Map& index);
  public: // Destructors
    virtual ~DwFunctionIndexer();
  public: // Virtual methods
    virtual void visit(DwSubprogram& s);
};

/**
 * @brief A visitor for indexing global variables.
 *
 * Indexes global variables, i.e., creates a map mapping addresses of variables
 *   to DWARF variable debugging information entry objects.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-09
 * @date      Last Update 2011-11-11
 * @version   0.1
 */
class DwGlobalVariableIndexer : public DwDieVisitor
{
  private: // User-defined variables
    Dwarf_Variable_Map& m_index; //!< A table mapping addresses to variables.
  public: // Constructors
    DwGlobalVariableIndexer(Dwarf_Variable_Map& index);
  public: // Destructors
    virtual ~DwGlobalVariableIndexer();
  public: // Virtual methods
    virtual void visit(DwVariable& v);
};

#endif /* __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__ */

/** End of file pin_dw_visitors.h **/
