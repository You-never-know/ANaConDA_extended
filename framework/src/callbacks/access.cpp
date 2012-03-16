/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-03-16
 * @version   0.4.1
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
  LOCATION loc; //!< A source code location where the access originates from.

  /**
   * Constructs a MemoryAccess_s object.
   */
  MemoryAccess_s() : addr(0), size(0), var(), loc() {}
} MemoryAccess;

// Declarations of static functions (usable only within this module)
static VOID deleteMemoryAccesses(void* memoryAccesses);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_memoryAccessesTlsKey = PIN_CreateThreadDataKey(deleteMemoryAccesses);

  typedef std::vector< MEMREAD1FUNPTR > MemRead1FunPtrVector;
  typedef std::vector< MEMREAD2FUNPTR > MemRead2FunPtrVector;
  typedef std::vector< MEMWRITE1FUNPTR > MemWrite1FunPtrVector;
  typedef std::vector< MEMWRITE2FUNPTR > MemWrite2FunPtrVector;

  MemRead1FunPtrVector g_beforeMemRead1Vector;
  MemRead2FunPtrVector g_beforeMemRead2Vector;
  MemWrite1FunPtrVector g_beforeMemWrite1Vector;
  MemWrite2FunPtrVector g_beforeMemWrite2Vector;

  MemRead1FunPtrVector g_afterMemRead1Vector;
  MemRead2FunPtrVector g_afterMemRead2Vector;
  MemWrite1FunPtrVector g_afterMemWrite1Vector;
  MemWrite2FunPtrVector g_afterMemWrite2Vector;
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
VOID beforeMemoryRead1(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  for (MemRead1FunPtrVector::iterator it = g_beforeMemRead1Vector.begin();
    it != g_beforeMemRead1Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, memAcc.var);
  }
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
VOID beforeMemoryRead2(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  // Analysis functions need to get the client lock before accessing locations
  PIN_LockClient();

  // Get the source code location where the memory access originates from
  PIN_GetSourceLocation(insAddr, NULL, &memAcc.loc.line, &memAcc.loc.file);

  // Do not hold the client lock longer that is absolutely necessary
  PIN_UnlockClient();

  for (MemRead2FunPtrVector::iterator it = g_beforeMemRead2Vector.begin();
    it != g_beforeMemRead2Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, memAcc.var, memAcc.loc);
  }

  for (MemRead1FunPtrVector::iterator it = g_beforeMemRead1Vector.begin();
    it != g_beforeMemRead1Vector.end(); it++)
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
VOID beforeMemoryWrite1(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  for (MemWrite1FunPtrVector::iterator it = g_beforeMemWrite1Vector.begin();
    it != g_beforeMemWrite1Vector.end(); it++)
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
VOID beforeMemoryWrite2(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  // Get the variable stored on the accessed address
  getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);

  // Analysis functions need to get the client lock before accessing locations
  PIN_LockClient();

  // Get the source code location where the memory access originates from
  PIN_GetSourceLocation(insAddr, NULL, &memAcc.loc.line, &memAcc.loc.file);

  // Do not hold the client lock longer that is absolutely necessary
  PIN_UnlockClient();

  for (MemWrite2FunPtrVector::iterator it = g_beforeMemWrite2Vector.begin();
    it != g_beforeMemWrite2Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, addr, size, memAcc.var, memAcc.loc);
  }

  for (MemWrite1FunPtrVector::iterator it = g_beforeMemWrite1Vector.begin();
    it != g_beforeMemWrite1Vector.end(); it++)
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
VOID afterMemoryRead1(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (MemRead1FunPtrVector::iterator it = g_afterMemRead1Vector.begin();
    it != g_afterMemRead1Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
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
VOID afterMemoryRead2(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (MemRead2FunPtrVector::iterator it = g_afterMemRead2Vector.begin();
    it != g_afterMemRead2Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.loc);
  }

  for (MemRead1FunPtrVector::iterator it = g_afterMemRead1Vector.begin();
    it != g_afterMemRead1Vector.end(); it++)
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
VOID afterMemoryWrite1(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (MemWrite1FunPtrVector::iterator it = g_afterMemWrite1Vector.begin();
    it != g_afterMemWrite1Vector.end(); it++)
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
VOID afterMemoryWrite2(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  for (MemWrite2FunPtrVector::iterator it = g_afterMemWrite2Vector.begin();
    it != g_afterMemWrite2Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.loc);
  }

  for (MemWrite1FunPtrVector::iterator it = g_afterMemWrite1Vector.begin();
    it != g_afterMemWrite1Vector.end(); it++)
  { // Call all callback functions registered by the user (used analyser)
    (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
}

/**
 * Initialises TLS (thread local storage) data for a thread.
 *
 * @param tid A number identifying the thread.
 * @param ctxt A structure containing the initial register state of the thread.
 * @param flags OS specific thread flags.
 * @param v Data passed to the callback registration function.
 */
VOID initMemoryAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
  // There can only be two simultaneous memory accesses at one time, because no
  // Intel instruction have more that 2 memory accesses, this will suffice then
  PIN_SetThreadData(g_memoryAccessesTlsKey, new MemoryAccess[2], tid);
}

/**
 * Setups memory access callback functions and their types.
 *
 * @param mais An object containing memory access instrumentation settings.
 */
VOID setupMemoryAccessSettings(MemoryAccessInstrumentationSettings& mais)
{
  if (!g_beforeMemRead2Vector.empty())
  { // Requires TID, ADDR, SIZE, VARIABLE and LOCATION
    mais.reads.beforeCallback = (AFUNPTR)beforeMemoryRead2;
    mais.reads.beforeCallbackType = CLBK_TYPE2;
  }
  else if (!g_beforeMemRead1Vector.empty())
  { // Requires TID, ADDR, SIZE and VARIABLE
    mais.reads.beforeCallback = (AFUNPTR)beforeMemoryRead1;
    mais.reads.beforeCallbackType = CLBK_TYPE1;
  }

  if (!g_beforeMemWrite2Vector.empty())
  { // Requires TID, ADDR, SIZE, VARIABLE and LOCATION
    mais.writes.beforeCallback = (AFUNPTR)beforeMemoryWrite2;
    mais.writes.beforeCallbackType = CLBK_TYPE2;
  }
  else if (!g_beforeMemWrite1Vector.empty())
  { // Requires TID, ADDR, SIZE and VARIABLE
    mais.writes.beforeCallback = (AFUNPTR)beforeMemoryWrite1;
    mais.writes.beforeCallbackType = CLBK_TYPE1;
  }

  if (!g_afterMemRead2Vector.empty())
  { // Requires TID, ADDR, SIZE, VARIABLE and LOCATION
    mais.reads.afterCallback = (AFUNPTR)afterMemoryRead2;
    mais.reads.afterCallbackType = CLBK_TYPE2;
  }
  else if (!g_afterMemRead1Vector.empty())
  { // Requires TID, ADDR, SIZE and VARIABLE
    mais.reads.afterCallback = (AFUNPTR)afterMemoryRead1;
    mais.reads.afterCallbackType = CLBK_TYPE1;
  }

  if (!g_afterMemWrite2Vector.empty())
  { // Requires TID, ADDR, SIZE, VARIABLE and LOCATION
    mais.writes.afterCallback = (AFUNPTR)afterMemoryWrite2;
    mais.writes.afterCallbackType = CLBK_TYPE2;
  }
  else if (!g_afterMemWrite1Vector.empty())
  { // Requires TID, ADDR, SIZE and VARIABLE
    mais.writes.afterCallback = (AFUNPTR)afterMemoryWrite1;
    mais.writes.afterCallbackType = CLBK_TYPE1;
  }

  // If no callback is registered, there is no need to instrument the accesses
  mais.instrument
    = (bool)mais.reads.beforeCallback
    | (bool)mais.reads.afterCallback
    | (bool)mais.writes.beforeCallback
    | (bool)mais.writes.afterCallback;
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREAD1FUNPTR callback)
{
  g_beforeMemRead1Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREAD2FUNPTR callback)
{
  g_beforeMemRead2Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITE1FUNPTR callback)
{
  g_beforeMemWrite1Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITE2FUNPTR callback)
{
  g_beforeMemWrite2Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREAD1FUNPTR callback)
{
  g_afterMemRead1Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREAD2FUNPTR callback)
{
  g_afterMemRead2Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITE1FUNPTR callback)
{
  g_afterMemWrite1Vector.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITE2FUNPTR callback)
{
  g_afterMemWrite2Vector.push_back(callback);
}

/** End of file access.cpp **/
