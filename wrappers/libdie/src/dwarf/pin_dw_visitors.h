/**
 * @brief A file containing definitions of classes for visiting DWARF entries.
 *
 * A file containing definitions of classes for visiting various DWARF entries.
 *
 * @file      pin_dw_visitors.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-15
 * @date      Last Update 2013-03-08
 * @version   0.2
 */

#ifndef __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__
  #define __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__

#include <map>

#include "dwarf/dw_classes.h"
#include "dwarf/dw_visitors.h"

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
