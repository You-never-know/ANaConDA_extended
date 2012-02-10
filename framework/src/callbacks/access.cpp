/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-02-10
 * @version   0.2.1
 */

#include "access.h"

#include "pin_die.h"

namespace
{ // Static global variables (usable only within this module)
  typedef std::vector< TYPE1READFUNPTR > Type1ReadFunPtrVector;
  typedef std::vector< TYPE1WRITEFUNPTR > Type1WriteFunPtrVector;

  Type1ReadFunPtrVector g_beforeType1ReadVector;
  Type1WriteFunPtrVector g_beforeType1WriteVector;
}

/**
 * Gets a variable stored at a specific memory location.
 *
 * @param rtnAddr An address of the routine which accessed the variable.
 * @param insAddr An address of the instruction which accessed the variable.
 * @param accessedAddr An address at which is the accessed variable stored.
 * @param size A size in bytes accessed (might be less that the size of the
 *   accessed variable).
 * @param registers A structure containing register values.
 * @param variable A structure where the information about the accessed
 *   variable should be stored.
 */
inline
void getVariable(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT accessedAddr,
  INT32 size, CONTEXT* registers, VARIABLE& variable)
{
  // Get the name and type of the variable, if part of an object or a structure
  // is accessed and the part do not correspond with any member (accessed part
  // of the member, more members etc.), the name and type of the object or the
  // structure is returned with an offset within this object or structure at
  // which the accessed data are stored
  DIE_GetVariable(rtnAddr, insAddr, accessedAddr, size, registers, /* input */
    variable.name, variable.type, &variable.offset); /* output */
}

/**
 * Calls all callback functions registered by a user to be called before reading
 *   from a memory.
 *
 * @note This function is called before some instruction reads from a memory.
 *
 * @param tid A number identifying the thread which performed the read.
 * @param addr An address of the data read.
 * @param size A size in bytes of the data read.
 * @param rtnAddr An address of the routine which read from the memory.
 * @param insAddr An address of the instruction which read from the memory.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Helper variables
  VARIABLE variable;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, variable);

  for (Type1ReadFunPtrVector::iterator it = g_beforeType1ReadVector.begin();
    it != g_beforeType1ReadVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, variable);
  }
}

/**
 * Calls all callback functions registered by a user to be called before writing
 *   to a memory.
 *
 * @note This function is called before some instruction writes to a memory.
 *
 * @param tid A number identifying the thread which performed the write.
 * @param addr An address of the data written.
 * @param size A size in bytes of the data written.
 * @param rtnAddr An address of the routine which written to the memory.
 * @param insAddr An address of the instruction which written to the memory.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Helper variables
  VARIABLE variable;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, variable);

  for (Type1WriteFunPtrVector::iterator it = g_beforeType1WriteVector.begin();
    it != g_beforeType1WriteVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, variable);
  }
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(TYPE1READFUNPTR callback)
{
  g_beforeType1ReadVector.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(TYPE1WRITEFUNPTR callback)
{
  g_beforeType1WriteVector.push_back(callback);
}

/** End of file access.cpp **/
