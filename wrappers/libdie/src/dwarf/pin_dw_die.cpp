/**
 * @brief A file containing implementation of classes and functions for
 *   accessing DWARF debugging information in PIN.
 *
 * A file containing implementation of classes and functions for accessing
 *   DWARF debugging information in PIN.
 *
 * @file      pin_dw_die.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-12
 * @date      Last Update 2011-11-11
 * @version   0.1.0.1
 */

#include "pin_dw_die.h"

#include <map>

#include "boost/assign/list_of.hpp"

#include "die.h"

#include "dwarf/dw_die.h"

#include "pin_dw_visitors.h"

namespace
{ // Static global variables (usable only within this module)
  std::map< std::string, DebugInfo* > g_dbgInfoMap;
  Dwarf_Function_Map g_functionMap;
  Dwarf_Variable_Map g_globalVarMap;

  /**
   * @brief A table mapping DWARF register numbers to corresponding PIN AMD64
   *   register numbers.
   */
  std::map< int, REG >
    g_dwAMD64RegTable = boost::assign::map_list_of
      (0, REG_RAX)
      (1, REG_RDX)
      (2, REG_RCX)
      (3, REG_RBX)
      (4, REG_RSI)
      (5, REG_RDI)
      (6, REG_RBP)
      (7, REG_RSP)
      (8, REG_R8)
      (9, REG_R9)
      (10, REG_R10)
      (11, REG_R11)
      (12, REG_R12)
      (13, REG_R13)
      (14, REG_R14)
      (15, REG_R15)
      (16, REG_INVALID_) // Return address (RA)
      (17, REG_XMM0)
      (18, REG_XMM1)
      (19, REG_XMM2)
      (20, REG_XMM3)
      (21, REG_XMM4)
      (22, REG_XMM5)
      (23, REG_XMM6)
      (24, REG_XMM7)
      (25, REG_XMM8)
      (26, REG_XMM9)
      (27, REG_XMM10)
      (28, REG_XMM11)
      (29, REG_XMM12)
      (30, REG_XMM13)
      (31, REG_XMM14)
      (32, REG_XMM15)
      (32, REG_XMM15)
      (33, REG_ST0)
      (34, REG_ST1)
      (35, REG_ST2)
      (36, REG_ST3)
      (37, REG_ST4)
      (38, REG_ST5)
      (39, REG_ST6)
      (40, REG_ST7)
      (41, REG_MM0)
      (42, REG_MM1)
      (43, REG_MM2)
      (44, REG_MM3)
      (45, REG_MM4)
      (46, REG_MM5)
      (47, REG_MM6)
      (48, REG_MM7)
      (49, REG_RFLAGS)
      (50, REG_SEG_ES)
      (51, REG_SEG_CS)
      (52, REG_SEG_SS)
      (53, REG_SEG_DS)
      (54, REG_SEG_FS)
      (55, REG_SEG_GS)
      (56, REG_INVALID_) // Reserved
      (57, REG_INVALID_) // Reserved
      (58, REG_SEG_FS_BASE)
      (59, REG_SEG_GS_BASE)
      (60, REG_INVALID_) // Reserved
      (61, REG_INVALID_) // Reserved
      (62, REG_TR)
      (63, REG_LDTR)
      (64, REG_MXCSR)
      (65, REG_FPCW)
      (66, REG_FPSW);
}

/**
 * @brief A class for retrieving values of DWARF AMD64 registers.
 *
 * Retrieves values of DWARF registers on the AMD64 architecture.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-19
 * @date      Last Update 2011-10-12
 * @version   0.1
 */
class DwAMD64Registers : public DwRegisters
{
  private: // User-defined variables
    const CONTEXT *m_registers; //!< A structure containing register values.
  public: // Constructors
    /**
     * Constructs a DwAMD64Registers object.
     *
     * @param registers A structure containing register values.
     */
    DwAMD64Registers(const CONTEXT *registers) : m_registers(registers) {}
  public: // Destructors
    /**
     * Destroys a DwAMD64Registers object.
     */
    virtual ~DwAMD64Registers() {}
  public: // Virtual methods for retrieving values of DWARF registers
    /**
     * Gets a value of a DWARF register.
     *
     * @param number A number identifying the DWARF register.
     * @return The value of the DWARF register or @em 0 if the value cannot be
     *   retrieved.
     */
    Dwarf_Addr getValue(int number)
    {
      // Currently the DWARF location expressions use only registers 0 to 31
      assert(0 <= number && number <= 31);

      // Get the value of the specified DWARF register and return it
      return PIN_GetContextReg(m_registers, g_dwAMD64RegTable[number]);
    }
};

/**
 * Opens an image (executable, shared object, dynamic library, ...).
 *
 * @param image An object representing the image.
 */
void dwarf_open(IMG image)
{
  // Get the name of the image
  std::string imgName = IMG_Name(image);

  if (g_dbgInfoMap.find(imgName) == g_dbgInfoMap.end())
  { // Extract the DWARF debugging information from the specified image
    DebugInfo* dbgInfo = DIE_GetDebugInfo(imgName);
    // Index all functions in the specified image
    DwFunctionIndexer functionIndexer(g_functionMap);
    // The debug info must always be DWARF debug info here, static cast to it
    static_cast< DwarfDebugInfo* >(dbgInfo)->accept(functionIndexer);
    // Index all global variables in the specified image
    DwGlobalVariableIndexer globalVarIndexer(g_globalVarMap);
    // The debug info must always be DWARF debug info here, static cast to it
    static_cast< DwarfDebugInfo* >(dbgInfo)->accept(globalVarIndexer);
    // Save the extracted DWARF debugging information
    g_dbgInfoMap[imgName] = dbgInfo;
  }

  // Print the DWARF debugging info, TODO: move to separate function
  g_dbgInfoMap[imgName]->printDebugInfo();
}

/**
 * Gets a variable stored on an accessed address.
 *
 * @param rtnAddr An address of the routine in which the variable was accessed.
 * @param insnAddr An address of the instruction which accessed the variable.
 * @param accessAddr The accessed address.
 * @param size A number of bytes accessed.
 * @param registers A structure containing register values.
 * @param name A reference to a string to which will be stored the name of the
 *   variable.
 * @param type A reference to a string to which will be stored the type of the
 *   variable.
 * @param offset A pointer to an integer to which will be stored the offset if
 *   only part of the variable was accessed.
 * @return @em True if the variable was found, @em false otherwise.
 */
bool dwarf_get_variable(ADDRINT rtnAddr, ADDRINT insnAddr, ADDRINT accessAddr,
  INT32 size, const CONTEXT *registers, std::string& name, std::string& type,
  UINT32 *offset)
{
  // Check if the variable accessed is not a global variable first
  Dwarf_Variable_Map::iterator it = g_globalVarMap.find(accessAddr);

  if (it != g_globalVarMap.end())
  { // A global variable is accessed, get its name and type
    name = it->second->getName();
    type = it->second->getDeclarationSpecifier();

    // Currently inner accesses within global variables are not supported
    if (offset != NULL) *offset = 0;
  }

  if (g_functionMap.find(rtnAddr) == g_functionMap.end())
  { // No information about variables in the specified routine
    return false;
  }

  // Helper variables
  Dwarf_Off dObjOffset = 0;

  // Create an object for retrieving values of DWARF registers
  DwAMD64Registers dwRegisters(registers);

  // Find the data object stored at the accessed address
  DwDie *die = g_functionMap[rtnAddr]->findDataObject(accessAddr, insnAddr,
    dwRegisters, &dObjOffset);

  // If offset is required, save it to the output parameter
  if (offset != NULL) *offset = dObjOffset;

  if (die != NULL)
  { // If some data object is found, store info about it to the output params
    name = die->getName();

    if (die->getTag() == DW_TAG_variable)
    { // A variable is stored at the accessed address
      DwVariable *var = static_cast< DwVariable* >(die);

      // Save the type of the variable
      type = var->getDeclarationSpecifier();

      if (var->isClass())
      { // Variable is a class, try to localise the concrete member accessed
        DwMember *member = static_cast< DwClassType* >(var->getDataType())
          ->getMember(dObjOffset);

        if (member != NULL && member->getSize() == (Dwarf_Unsigned)size)
        { // Accessed a concrete member, save the name of the concrete member
          name = var->getDeclarationSpecifier() + "." + name + "."
            + static_cast< DwClassType* >(var->getDataType())
                ->getMemberName(dObjOffset);
          // Save the type of the concrete member
          type = member->getDeclarationSpecifier();
          // Offset now must be 0 (offset from the member to itself is 0)
          if (offset != NULL) *offset = 0;
        }
        else
        { // Accessed more or a part of a member, don't know, save global offset
          if (offset != NULL) *offset = dObjOffset;
        }
      }
    }

    // The data object stored at the accessed address was found
    return true;
  }

  // No data object stored at the accessed address was found
  return false;
}

/** End of file pin_dw_die.cpp **/
