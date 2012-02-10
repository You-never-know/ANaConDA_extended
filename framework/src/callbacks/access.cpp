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
 * @version   0.3
 */

#include "access.h"

#include "pin_die.h"

/**
 * @brief A structure containing information about a memory access.
 */
typedef struct MemoryAccess_s
{
  ADDRINT addr; //!< An accessed address.
  UINT32 size; //!< A size in bytes accessed.
  VARIABLE var; //!< A variable accessed.

  /**
   * Constructs a MemoryAccess_s object.
   */
  MemoryAccess_s() : addr(0), size(0), var() {}
} MemoryAccess;

// Declarations of static functions (usable only within this module)
static VOID deleteMemoryAccesses(void* memoryAccesses);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_memoryAccessesTlsKey = PIN_CreateThreadDataKey(deleteMemoryAccesses);

  typedef std::vector< TYPE1READFUNPTR > Type1ReadFunPtrVector;
  typedef std::vector< TYPE1WRITEFUNPTR > Type1WriteFunPtrVector;

  Type1ReadFunPtrVector g_beforeType1ReadVector;
  Type1WriteFunPtrVector g_beforeType1WriteVector;

  Type1ReadFunPtrVector g_afterType1ReadVector;
  Type1WriteFunPtrVector g_afterType1WriteVector;
}

/**
 * Deletes an array of memory accesses created during thread start.
 *
 * @param memoryAccesses An array of memory accesses.
 */
VOID deleteMemoryAccesses(void* memoryAccesses)
{
  delete[] static_cast< MemoryAccess* >(memoryAccesses);
}

/**
 * Gets the last memory accesses performed by a thread.
 *
 * @param tid A number identifying the thread.
 * @return The last memory accesses performed by the thread.
 */
inline
MemoryAccess* getLastMemoryAccesses(THREADID tid)
{
  return static_cast< MemoryAccess* >(PIN_GetThreadData(g_memoryAccessesTlsKey,
    tid));
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
 * Initialises TLS (thread local storage) data for a thread.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID initAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // There can only be two simultaneous memory accesses at one time, because no
  // Intel instruction have more that 2 memory accesses, this will suffice then
  PIN_SetThreadData(g_memoryAccessesTlsKey, new MemoryAccess[2], tid);
}

/**
 * Calls all callback functions registered by a user to be called before reading
 *   from a memory.
 *
 * @note This function is called before an instruction reads from a memory.
 *
 * @param tid A number identifying the thread which performed the read.
 * @param addr An address of the data read.
 * @param size A size in bytes of the data read.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 * @param rtnAddr An address of the routine which read from the memory.
 * @param insAddr An address of the instruction which read from the memory.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  for (Type1ReadFunPtrVector::iterator it = g_beforeType1ReadVector.begin();
    it != g_beforeType1ReadVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, memAcc.var);
  }
}

/**
 * Calls all callback functions registered by a user to be called before writing
 *   to a memory.
 *
 * @note This function is called before an instruction writes to a memory.
 *
 * @param tid A number identifying the thread which performed the write.
 * @param addr An address of the data written.
 * @param size A size in bytes of the data written.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 * @param rtnAddr An address of the routine which written to the memory.
 * @param insAddr An address of the instruction which written to the memory.
 * @param registers A structure containing register values.
 */
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  for (Type1WriteFunPtrVector::iterator it = g_beforeType1WriteVector.begin();
    it != g_beforeType1WriteVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, memAcc.var);
  }
}

/**
 * Calls all callback functions registered by a user to be called after reading
 *   from a memory.
 *
 * @note This function is called after an instruction reads from a memory.
 *
 * @param tid A number identifying the thread which performed the read.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 */
VOID afterMemoryRead(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (Type1ReadFunPtrVector::iterator it = g_afterType1ReadVector.begin();
    it != g_afterType1ReadVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
}

/**
 * Calls all callback functions registered by a user to be called after writing
 *   to a memory.
 *
 * @note This function is called after an instruction writes to a memory.
 *
 * @param tid A number identifying the thread which performed the write.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 */
VOID afterMemoryWrite(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (Type1WriteFunPtrVector::iterator it = g_afterType1WriteVector.begin();
    it != g_afterType1WriteVector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
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

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(TYPE1READFUNPTR callback)
{
  g_afterType1ReadVector.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(TYPE1WRITEFUNPTR callback)
{
  g_afterType1WriteVector.push_back(callback);
}

/** End of file access.cpp **/
