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
 * @date      Last Update 2011-09-15
 * @version   0.1
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

/** End of file pin_dw_visitors.cpp **/
