/**
 * @brief A file containing definitions of classes for visiting DWARF entries.
 *
 * A file containing definitions of classes for visiting various DWARF entries.
 *
 * @file      pin_dw_visitors.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-15
 * @date      Last Update 2011-09-15
 * @version   0.1
 */

#ifndef __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__
  #define __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__

#include <map>

#include "dwarf/dw_classes.h"
#include "dwarf/dw_visitors.h"

typedef std::map< Dwarf_Addr, DwSubprogram* > Dwarf_Function_Map;

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

#endif /* __LIBPIN_DIE__DWARF__PIN_DW_VISITORS_H__ */

/** End of file pin_dw_visitors.h **/
