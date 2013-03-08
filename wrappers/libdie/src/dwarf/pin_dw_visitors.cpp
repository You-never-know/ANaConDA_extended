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
