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
 * @brief A file containing definitions of classes for extracting, holding and
 *   manipulating the DWARF debugging information.
 *
 * A file containing definitions of classes for extracting, holding and
 *   manipulating the DWARF debugging information.
 *
 * @file      dw_die.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-09-15
 * @version   0.1.2
 */

#ifndef __LIBDIE__DWARF__DW_DIE_H__
  #define __LIBDIE__DWARF__DW_DIE_H__

#include <list>

#include "boost/shared_ptr.hpp"

#include "../die.h"

#include "dw_classes.h"

class DwarfDebugInfoExtractor;

/**
 * @brief A class for holding the DWARF debugging information.
 *
 * Holds the DWARF debugging information and defines methods for accessing it.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-09-15
 * @version   0.1.2
 */
class DwarfDebugInfo : public DebugInfo
{
    friend class DwarfDebugInfoExtractor;
  private:
    Dwarf_Debug m_dbg; //!< A DWARF handle for accessing debugging records.
    /**
     * @brief A list containing all DWARF compile units present in a debugging
     *   information section of a file.
     */
    std::list< boost::shared_ptr< DwCompileUnit > > m_compileUnitList;
  public: // Constructors
    DwarfDebugInfo() {}
    DwarfDebugInfo(const DwarfDebugInfo& di) : m_dbg(di.m_dbg),
      m_compileUnitList(di.m_compileUnitList) {}
  public: // Destructors
    virtual ~DwarfDebugInfo() {}
  public: // Member methods for visiting DWARF CUs
    void accept(DwDieVisitor& visitor);
    void accept(DwDieTreeTraverser& traverser);
  public: // Virtual methods
    void printDebugInfo();
  public: // Member methods
    void printVariables();
};

/**
 * @brief A class for extracting DWARF debug information from a file.
 *
 * Contains methods for extracting DWARF debug information from a file.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-08-26
 * @version   0.1.1
 */
class DwarfDebugInfoExtractor
{
  private: // Static attributes
    /**
     * @brief A singleton instance.
     */
    static boost::shared_ptr< DwarfDebugInfoExtractor > ms_instance;
  private:
    std::map< std::string, boost::shared_ptr < DwarfDebugInfo > > m_dbgInfos;
  public: // Constructors
    DwarfDebugInfoExtractor();
  public: // Destructors
    ~DwarfDebugInfoExtractor();
  public: // Static methods
    static DwarfDebugInfoExtractor *Get();
  public: // Member methods
    DwarfDebugInfo *getDebugInfo(std::string filename);
  private: // Internal helper methods
    DwarfDebugInfo *extractDebugInfo(std::string filename);
    DwDie* extractDebugInfoEntry(Dwarf_Die& die, Dwarf_Debug& dbg);
};

#endif /* __LIBDIE__DWARF__DW_DIE_H__ */

/** End of file dw_die.h **/
