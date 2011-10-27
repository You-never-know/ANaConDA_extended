/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2011-10-27
 * @version   0.1.1
 */

#include "access.h"

#include <assert.h>

#include <iostream>

#include <boost/lexical_cast.hpp>

#include "pin_die.h"

/**
 * Reads a raw data stored at a specific memory location.
 *
 * @param addr An address at which are the data stored.
 * @param size A size in bytes of the data to be read.
 * @return A 64-bit number representing the read 1 to 8 bytes of data.
 */
inline
UINT64 readRawMemory(ADDRINT addr, INT32 size)
{
  assert(size <= 8); // No more then 8 bytes should be read

  switch (size)
  {
    case 1: // One byte of memory should be read
      return static_cast< UINT64 >(*reinterpret_cast< UINT8* >(addr));
    case 2: // Two bytes of memory should be read
      return static_cast< UINT64 >(*reinterpret_cast< UINT16* >(addr));
    case 4: // Four bytes of memory should be read
      return static_cast< UINT64 >(*reinterpret_cast< UINT32* >(addr));
    case 8: // Eight bytes of memory should be read
      return static_cast< UINT64 >(*reinterpret_cast< UINT64* >(addr));
    default: // Other number of bytes read ? Suspicious, probably some error
      LOG("Cannot read " + boost::lexical_cast< string >(size)
        + " bytes of memory.\n");
      return 0; // No data type to read this number of bytes
  }
}

/**
 * Gets a declaration of a variable stored at a specific memory location.
 *
 * @param rtnAddr An address of the routine which accessed the variable.
 * @param insAddr An address of the instruction which accessed the variable.
 * @param accessedAddr An address at which is the accessed variable stored.
 * @param size A size in bytes accessed (might be less that the size of the
 *   accessed variable).
 * @param registers A structure containing register values.
 * @return A string containing the declaration of the accessed variable.
 */
inline
std::string getVariableDeclaration(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT accessedAddr, INT32 size, CONTEXT *registers)
{
  // Helper variables
  std::string name;
  std::string type;
  UINT32 offset = 0;

  // Get the name and type of the variable, if part of an object or a structure
  // is accessed and the part do not correspond with any member (accessed part
  // of the member, more members etc.), the name and type of the object or the
  // structure is returned with an offset within this object or structure at
  // which the accessed data are stored
  DIE_GetVariable(rtnAddr, insAddr, accessedAddr, size, registers, /* input */
    name, type, &offset); /* output */

  // Format the name, type and offset to a 'type name[+offset]' string
  return ((type.size() == 0) ? "" : type + " ")
    + ((name.empty()) ? "UNKNOWN" : name)
    + ((offset == 0) ? "" : "+" + boost::lexical_cast< string >(offset));
}

/**
 * Prints information about a read from a memory.
 *
 * @note This function is called before an instruction reads from a memory.
 *
 * @param rtnAddr An address of the routine which read from the memory.
 * @param insAddr An address of the instruction which read from the memory.
 * @param readAddr An address at which are the read data stored.
 * @param size A size in bytes of the data read.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryRead(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT readAddr,
  INT32 size, CONTEXT *registers)
{
  std::cout << "Read '" << readRawMemory(readAddr, size) << "' from "
    << getVariableDeclaration(rtnAddr, insAddr, readAddr, size, registers)
    << " [" << hex << readAddr << dec << "]\n";
}

/**
 * Prints information about a double read from a memory.
 *
 * @note This function is called before an instruction reads twice from
 *   a memory.
 *
 * @param rtnAddr An address of the routine which read from the memory.
 * @param insAddr An address of the instruction which read from the memory.
 * @param readAddr1 An address at which are the first read data stored.
 * @param readAddr2 An address at which are the second read data stored.
 * @param size A size in bytes of the data read (same for both reads).
 * @param registers A structure containing register values.
 */
VOID beforeMemoryRead2(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT readAddr1,
  ADDRINT readAddr2, INT32 size, CONTEXT *registers)
{
  std::cout << "Read '" << readRawMemory(readAddr1, size) << "' from "
    << getVariableDeclaration(rtnAddr, insAddr, readAddr1, size, registers)
    << " [" << hex << readAddr1 << dec << "]\n";
  std::cout << "Read '" << readRawMemory(readAddr2, size) << "' from "
    << getVariableDeclaration(rtnAddr, insAddr, readAddr2, size, registers)
    << " [" << hex << readAddr2 << dec << "]\n";
}

/**
 * Prints information about a write to a memory.
 *
 * @note This function is called before an instruction writes to a memory some
 *   value (stored somewhere else in the memory).
 *
 * @param rtnAddr An address of the routine which written to the memory.
 * @param insAddr An address of the instruction which written to the memory.
 * @param writtenAddr An address at which are the data written to.
 * @param size A size in bytes of the data written.
 * @param memAddr An address at which are the written data stored.
 * @param memSize A size in bytes of the data to be written.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryWrite(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT writtenAddr,
  INT32 size, ADDRINT memAddr, INT32 memSize, CONTEXT *registers)
{
  std::cout << "Written '" << readRawMemory(memAddr, memSize) << "' to "
    << getVariableDeclaration(rtnAddr, insAddr, writtenAddr, size, registers)
    << " [" << hex << writtenAddr << dec << "]\n";
}

/**
 * Prints information about a write to a memory.
 *
 * @note This function is called before an instruction writes to a memory some
 *   constant value (a value which is specified in the instruction itself).
 *
 * @param rtnAddr An address of the routine which written to the memory.
 * @param insAddr An address of the instruction which written to the memory.
 * @param writtenAddr An address at which are the data written to.
 * @param size A size in bytes of the data written.
 * @param value The data written.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryWriteValue(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, ADDRINT value, CONTEXT *registers)
{
  std::cout << "Written '" << value << "' to "
    << getVariableDeclaration(rtnAddr, insAddr, writtenAddr, size, registers)
    << " [" << hex << writtenAddr << dec << "]\n";
}

/** End of file access.cpp **/
