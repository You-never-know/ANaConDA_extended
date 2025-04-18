/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of libdie.
 *
 * libdie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libdie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdie. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing definitions of classes for visiting DWARF entries.
 *
 * A file containing definitions of classes for visiting various DWARF entries.
 *
 * @file      dw_visitors.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-23
 * @date      Last Update 2011-09-19
 * @version   0.2.4
 */

#ifndef __LIBDIE__DWARF__DW_VISITORS_H__
  #define __LIBDIE__DWARF__DW_VISITORS_H__

#include <iostream>

#ifndef __LIBDIE__DWARF__DW_CLASSES_H__
  #include "dw_classes.h"
#else
  typedef struct Dwarf_Source_File_List_s Dwarf_Source_File_List;
  typedef struct Dwarf_Attribute_Value_s Dwarf_Attribute_Value;

  class DwDie;
  class DwFormalParameter;
  class DwCompileUnit;
  class DwSubprogram;
  class DwVariable;
#endif

/**
 * @brief A generic visitor for visiting DWARF debugging information entries.
 *
 * Visits various DWARF debugging information entries.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-23
 * @date      Last Update 2011-09-19
 * @version   0.1.3
 */
class DwDieVisitor
{
  protected: // Constructors
    DwDieVisitor();
  public: // Destructors
    virtual ~DwDieVisitor();
  public: // Virtual methods
    virtual void visit(DwDie& die);
    virtual void visit(DwFormalParameter& fp);
    virtual void visit(DwCompileUnit& cu);
    virtual void visit(DwSubprogram& s);
    virtual void visit(DwVariable& v);
};

/**
 * @brief A visitor for linking DWARF references to DWARF DIE objects.
 *
 * Links DWARF references (offsets) to DWARF debugging information entry objects
 *   which resides at these offsets.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-31
 * @date      Last Update 2011-09-05
 * @version   0.1
 */
class DwReferenceLinker : public DwDieVisitor
{
  private: // Computed variables
    /**
     * @brief A global offset of the currently processed DWARF compilation unit
     *   DIE. This offset will be used to compute the global offset of the DIEs.
     */
    Dwarf_Off m_currentCUGlobalOffset;
    /**
     * @brief A table mapping global offsets to DWARF DIE objects which reside
     *   at these offsets.
     */
    std::map< Dwarf_Off, DwDie* > m_references;
    /**
     * @brief A table containing all attributes which referenced DWARF DIE
     *   objects which have not been visited yet. These attributes are updated
     *   when the referenced DWARF DIE object is visited.
     */
    std::map< Dwarf_Off, std::list< Dwarf_Attribute_Value* > > m_attributes;
  public: // Destructors
    virtual ~DwReferenceLinker();
  public: // Virtual methods
    virtual void visit(DwDie& die);
    virtual void visit(DwCompileUnit& cu);
  private: // Internal helper methods
    void updateReferences(DwDie& die);
};

/**
 * @brief A visitor for evaluating source file indexes.
 *
 * Evaluates source file indexes, i.e., replaces indexes to a table containing
 *   the names of the source files with the actual pointers to these names.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-08
 * @date      Last Update 2011-09-08
 * @version   0.1
 */
class DwSourceFileIndexEvaluator : public DwDieVisitor
{
  private: // Retrieved variables
    const Dwarf_Source_File_List *m_srcFileList;
  public: // Destructors
    virtual ~DwSourceFileIndexEvaluator();
  public: // Virtual methods
    virtual void visit(DwDie& die);
    virtual void visit(DwCompileUnit& cu);
  private: // Internal helper methods
    void replaceIndexesWithPointers(DwDie& die);
};

/**
 * @brief A visitor for finding data objects.
 *
 * Finds data objects (variables, formal parameters or constants).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-19
 * @date      Last Update 2011-09-19
 * @version   0.1
 */
class DwDataObjectFinder : public DwDieVisitor
{
  private: // Created variables
    /**
     * @brief A list containing found data objects (variables, formal parameters
     *   or constants).
     */
    std::list< DwDie* > m_dataObjectList;
  public: // Destructors
    virtual ~DwDataObjectFinder();
  public: // Virtual methods
    virtual void visit(DwFormalParameter& fp);
    virtual void visit(DwVariable& v);
  public: // Inline member methods
    /**
     * Gets a read-only list containing found data objects (variables, formal
     *   parameters or constants).
     *
     * @return A read-only list containing the found data objects.
     */
    const std::list< DwDie* >& getDataObjects() const
    {
      return m_dataObjectList;
    }
};

/**
 * @brief A visitor which prints information about variables to a stream.
 *
 * Prints information about variables to a stream.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-05
 * @date      Last Update 2011-09-06
 * @version   0.1
 */
class DwVariablePrinter : public DwDieVisitor
{
  protected: // User-defined variables
    std::ostream& m_stream; //!< A stream to which to print the variables.
  public: // Constructors
    DwVariablePrinter(std::ostream& stream = std::cout);
  public: // Destructors
    virtual ~DwVariablePrinter();
  public: // DwDieVisitor class virtual methods redefinition
    virtual void visit(DwVariable& v);
};

/**
 * @brief A visitor which traverses a DWARF debugging information entry tree.
 *
 * Traverses a DWARF debugging information entry tree.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-23
 * @date      Last Update 2011-08-24
 * @version   0.1
 */
class DwDieTreeTraverser : public DwDieVisitor
{
  protected: // Computed variables
    int m_depth; //!< A depth in which is the traverser currently present.
  protected: // Constructors
    DwDieTreeTraverser();
  public: // Destructors
    virtual ~DwDieTreeTraverser();
  public: // Inline member methods
    /**
     * Gets the current depth in which is the traverser currently present.
     *
     * @return The current depth in which is the traverser currently present.
     */
    int getDepth() { return m_depth; }

    /**
     * Increases the current depth in which is the traverser currently present.
     */
    void incDepth() { m_depth++; }

    /**
     * Decreases the current depth in which is the traverser currently present.
     */
    void decDepth() { m_depth--; }
};

/**
 * @brief A visitor which prints the DWARF debugging information to a stream.
 *
 * Prints the DWARF debugging information to a stream.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-23
 * @date      Last Update 2011-08-24
 * @version   0.1
 */
class DwDebugInfoPrinter : public DwDieTreeTraverser
{
  protected: // User-defined variables
    std::ostream& m_stream; //!< A stream to which to print the debug info.
  protected: // Computed variables
    size_t m_maxOffsetWidth; //!< A width of the maximum offset in a DWARF CU.
  public: // Constructors
    DwDebugInfoPrinter(std::ostream& stream = std::cout);
  public: // Destructors
    virtual ~DwDebugInfoPrinter();
  public: // DwDieVisitor class virtual methods redefinition
    virtual void visit(DwDie& die);
    virtual void visit(DwCompileUnit& cu);
  private:
    void printDie(DwDie& die);
};

#endif /* __LIBDIE__DWARF__DW_VISITORS_H__ */

/** End of file dw_visitors.h **/
