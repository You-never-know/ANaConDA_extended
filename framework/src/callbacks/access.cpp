/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-03-21
 * @version   0.5.0.1
 */

#include "access.h"

#include "pin_die.h"

/**
 * @brief A structure containing information about a memory access.
 */
typedef struct MemoryAccess_s
{
#ifdef DEBUG_MEMORY_ACCESSES
  ADDRINT rtn; //!< An address of the routine which performed the access.
  ADDRINT ins; //!< An address of the instruction which performed the access.
#endif
  ADDRINT addr; //!< An accessed address.
  UINT32 size; //!< A size in bytes accessed.
  VARIABLE var; //!< A variable accessed.
  LOCATION loc; //!< A source code location where the access originates from.

  /**
   * Constructs a MemoryAccess_s object.
   */
#ifdef DEBUG_MEMORY_ACCESSES
  MemoryAccess_s() : rtn(0), ins(0), addr(0), size(0), var(), loc() {}
#else
  MemoryAccess_s() : addr(0), size(0), var(), loc() {}
#endif
} MemoryAccess;

#ifdef DEBUG_MEMORY_ACCESSES
  #define ASSERT_MEMORY_ACCESS(var) \
    if (var.size != 0) \
    { \
      PIN_LockClient(); \
      RTN rtn = RTN_FindByAddress(var.rtn); \
      PIN_UnlockClient(); \
      RTN_Open(rtn); \
      for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) \
      { \
        if (INS_Address(ins) == var.ins) \
          CONSOLE("After callback not triggered for instruction " \
            + INS_Disassemble(ins) + "\n"); \
      } \
      RTN_Close(rtn); \
    } \
    else \
    { \
      var.rtn = rtnAddr; \
      var.ins = insAddr; \
    }
#else
  #define ASSERT_MEMORY_ACCESS(var) assert(var.size == 0);
#endif

// Declarations of static functions (usable only within this module)
static VOID deleteMemoryAccesses(void* memoryAccesses);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_memoryAccessesTlsKey = PIN_CreateThreadDataKey(deleteMemoryAccesses);
}

/**
 * @brief An enumeration describing the types of memory accesses.
 */
typedef enum AccessType_e
{
  READ, //!< A read access.
  WRITE //!< A write access.
} AccessType;

/**
 * @brief A structure containing traits information of callback functions.
 */
template < AccessType AT, CallbackType CT >
struct callback_traits
{
};

/**
 * @brief Defines traits information for a specific type of callback functions.
 *
 * @param access A type of the memory access triggering this type of callback
 *   functions (READ or WRITE access).
 * @param callback A type of the callback function (the number from the
 *   CLBK_TYPE<number>).
 */
#define DEFINE_CALLBACK_TRAITS(access, callback) \
  template<> \
  struct callback_traits< access, CLBK_TYPE##callback > \
  { \
    typedef MEM##access##callback##FUNPTR fun_ptr_type; \
    typedef std::vector< fun_ptr_type > container_type; \
    static container_type before; \
    static container_type after; \
  }; \
  callback_traits< access, CLBK_TYPE##callback >::container_type \
    callback_traits< access, CLBK_TYPE##callback >::before; \
  callback_traits< access, CLBK_TYPE##callback >::container_type \
    callback_traits< access, CLBK_TYPE##callback >::after

// Define traits information for the known types of callback functions
DEFINE_CALLBACK_TRAITS(READ, 1);
DEFINE_CALLBACK_TRAITS(READ, 2);
DEFINE_CALLBACK_TRAITS(WRITE, 1);
DEFINE_CALLBACK_TRAITS(WRITE, 2);

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
 * Calls all callback functions registered by a user to be called before
 *   accessing a memory.
 *
 * @note This function is called before an instruction accesses a memory.
 *
 * @tparam AT A type of the access.
 * @tparam CT A type of the callback function.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 */
template < AccessType AT, CallbackType CT >
inline
VOID beforeMemoryAccess(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Make sure we have triggered the after callback for the previous access
  ASSERT_MEMORY_ACCESS(memAcc);

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  if (CT & (CLBK_TYPE1 | CLBK_TYPE2))
  { // Get the variable stored on the accessed address
    getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);
  }

  if (CT & CLBK_TYPE2)
  { // Analysis functions need to get the client lock before accessing locations
    PIN_LockClient();

    // Get the source code location where the memory access originates from
    PIN_GetSourceLocation(insAddr, NULL, &memAcc.loc.line, &memAcc.loc.file);

    // Do not hold the client lock longer that is absolutely necessary
    PIN_UnlockClient();
  }

  if (CT & CLBK_TYPE2)
  { // Call all registered TYPE2 callback functions
    typedef callback_traits< AT, CLBK_TYPE2 > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, size, memAcc.var, memAcc.loc);
    }
  }

  if (CT & CLBK_TYPE1)
  { // Call all registered TYPE1 callback functions
    typedef callback_traits< AT, CLBK_TYPE1 > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, size, memAcc.var);
    }
  }
}

/**
 * Calls all callback functions registered by a user to be called after
 *   accessing a memory.
 *
 * @note This function is called after an instruction accesses a memory.
 *
 * @tparam AT A type of the access.
 * @tparam CT A type of the callback function.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 */
template < AccessType AT, CallbackType CT >
inline
VOID afterMemoryAccess(THREADID tid, UINT32 memOpIdx)
{
  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Make sure we have triggered the before callback for this access
  assert(memAcc.size != 0);

  if (CT & CLBK_TYPE2)
  { // Call all registered TYPE2 callback functions
    typedef callback_traits< AT, CLBK_TYPE2 > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.loc);
    }
  }

  if (CT & CLBK_TYPE1)
  { // Call all registered TYPE1 callback functions
    typedef callback_traits< AT, CLBK_TYPE1 > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
    }
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
}

// Helper macros defining parameters of the before/after callback functions
#define before_PARAMS THREADID tid, ADDRINT addr, UINT32 size, \
  UINT32 memOpIdx, ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers
#define after_PARAMS THREADID tid, UINT32 memOpIdx

/**
 * @brief Instantiates a concrete code of a callback function from a template.
 *
 * @param point A point at which is the callback function called (before/after).
 * @param access A type of the memory access triggering the callback function
 *   (READ/WRITE).
 * @param callback A type of the callback function (int from CLBK_TYPE<int>).
 */
#define INSTANTIATE_CALLBACK_FUNCTION(point, access, callback) \
  template VOID point##MemoryAccess< access, CLBK_TYPE##callback > \
    (point##_PARAMS)

// Instantiate callback functions called before memory accesses
INSTANTIATE_CALLBACK_FUNCTION(before, READ, 1);
INSTANTIATE_CALLBACK_FUNCTION(before, READ, 2);
INSTANTIATE_CALLBACK_FUNCTION(before, WRITE, 1);
INSTANTIATE_CALLBACK_FUNCTION(before, WRITE, 2);

// Instantiate callback functions called after memory accesses
INSTANTIATE_CALLBACK_FUNCTION(after, READ, 1);
INSTANTIATE_CALLBACK_FUNCTION(after, READ, 2);
INSTANTIATE_CALLBACK_FUNCTION(after, WRITE, 1);
INSTANTIATE_CALLBACK_FUNCTION(after, WRITE, 2);

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

// Helper macros for translating memory access enums to names of MAIS sections
#define MAIS_READ_SECTION reads
#define MAIS_WRITE_SECTION writes

/**
 * @brief Setups a callback function based on which type of callback function
 *   a user registered.
 *
 * @param point A point at which is the callback function called (before/after).
 * @param access A type of the memory access triggering the callback function
 *   (READ/WRITE).
 */
#define SETUP_CALLBACK_FUNCTION(point, access) \
  if (!callback_traits< access, CLBK_TYPE2 >::point.empty()) \
  { /* Requires TID, ADDR, SIZE, VARIABLE and LOCATION */ \
    mais.MAIS_##access##_SECTION.point##Callback = (AFUNPTR) \
      point##MemoryAccess< access, CLBK_TYPE2 >; \
    mais.MAIS_##access##_SECTION.point##CallbackType = CLBK_TYPE2; \
  } \
  else if (!callback_traits< access, CLBK_TYPE1 >::point.empty()) \
  { /* Requires TID, ADDR, SIZE and VARIABLE */ \
    mais.MAIS_##access##_SECTION.point##Callback = (AFUNPTR) \
      point##MemoryAccess< access, CLBK_TYPE1 >; \
    mais.MAIS_##access##_SECTION.point##CallbackType = CLBK_TYPE1; \
  }

/**
 * Setups memory access callback functions and their types.
 *
 * @param mais An object containing memory access instrumentation settings.
 */
VOID setupMemoryAccessSettings(MemoryAccessInstrumentationSettings& mais)
{
  // Setup a callback function which will be called before reads
  SETUP_CALLBACK_FUNCTION(before, READ);

  // Setup a callback function which will be called before writes
  SETUP_CALLBACK_FUNCTION(before, WRITE);

  // Setup a callback function which will be called after reads
  SETUP_CALLBACK_FUNCTION(after, READ);

  // Setup a callback function which will be called after writes
  SETUP_CALLBACK_FUNCTION(after, WRITE);

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
  callback_traits< READ, CLBK_TYPE1 >::before.push_back(callback);
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
  callback_traits< READ, CLBK_TYPE2 >::before.push_back(callback);
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
  callback_traits< WRITE, CLBK_TYPE1 >::before.push_back(callback);
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
  callback_traits< WRITE, CLBK_TYPE2 >::before.push_back(callback);
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
  callback_traits< READ, CLBK_TYPE1 >::after.push_back(callback);
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
  callback_traits< READ, CLBK_TYPE2 >::after.push_back(callback);
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
  callback_traits< WRITE, CLBK_TYPE1 >::after.push_back(callback);
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
  callback_traits< WRITE, CLBK_TYPE2 >::after.push_back(callback);
}

/** End of file access.cpp **/
