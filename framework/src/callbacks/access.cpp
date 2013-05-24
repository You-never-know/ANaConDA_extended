/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-05-24
 * @version   0.8.0.2
 */

#include "access.h"

#include "pin_die.h"

#include "../monitors/preds.h"
#include "../monitors/svars.hpp"

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
      RTN_Open(rtn); \
      for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) \
      { \
        if (INS_Address(ins) == var.ins) \
          CONSOLE("After callback not triggered for instruction " \
            + INS_Disassemble(ins) + "[" + hexstr(var.ins) + "] in function " \
            + RTN_Name(rtn) + " " + hexstr(var.rtn) + "]\n"); \
      } \
      PIN_UnlockClient(); \
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

// Helper macros
#define THREAD_DATA getThreadData(tid)
#define GET_REG_VALUE(reg) PIN_GetContextReg(registers, reg)

// Definitions of callback functions needed to instantiate traits for CLBK_NONE
typedef VOID (*MEMREADNONEFUNPTR)();
typedef VOID (*MEMWRITENONEFUNPTR)();
typedef VOID (*MEMUPDATENONEFUNPTR)();

// Declarations of static functions (usable only within this module)
static VOID deleteThreadData(void* threadData);
static VOID deleteMemoryAccesses(void* memoryAccesses);
static VOID deleteRepExecutedFlag(void* repExecutedFlag);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_threadDataTlsKey = PIN_CreateThreadDataKey(deleteThreadData);
  TLS_KEY g_memoryAccessesTlsKey = PIN_CreateThreadDataKey(deleteMemoryAccesses);
  TLS_KEY g_repExecutedFlagTlsKey = PIN_CreateThreadDataKey(deleteRepExecutedFlag);

  PredecessorsMonitor< FileWriter >* g_predsMon;
  SharedVariablesMonitor< FileWriter >* g_sVarsMon;
}

/**
 * @brief An enumeration describing the types of memory accesses.
 */
typedef enum AccessType_e
{
  READ,  //!< A read access.
  WRITE, //!< A write access.
  UPDATE //!< An atomic update access.
} AccessType;

/**
 * @brief A structure holding private data of a thread.
 */
typedef struct ThreadData_s
{
  ADDRINT splow; //!< The lowest value of stack pointer seen in the execution.

  /**
   * Constructs a ThreadData_s object.
   */
  ThreadData_s() : splow(-1) {}
} ThreadData;

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
 *   functions (items from the AccessType enumeration).
 * @param callback A type of the callback function (items from the CallbackType
 *   enumeration, but only the second part, e.g., the name from CLBK_<name>).
 */
#define DEFINE_CALLBACK_TRAITS(access, callback) \
  template<> \
  struct callback_traits< access, CLBK_##callback > \
  { \
    typedef MEM##access##callback##FUNPTR fun_ptr_type; \
    typedef std::vector< fun_ptr_type > container_type; \
    static container_type before; \
    static container_type after; \
  }; \
  callback_traits< access, CLBK_##callback >::container_type \
    callback_traits< access, CLBK_##callback >::before; \
  callback_traits< access, CLBK_##callback >::container_type \
    callback_traits< access, CLBK_##callback >::after

// Define traits information for the known types of callback functions
DEFINE_CALLBACK_TRAITS(READ, NONE);
DEFINE_CALLBACK_TRAITS(READ, A);
DEFINE_CALLBACK_TRAITS(READ, AV);
DEFINE_CALLBACK_TRAITS(READ, AVL);
DEFINE_CALLBACK_TRAITS(WRITE, NONE);
DEFINE_CALLBACK_TRAITS(WRITE, A);
DEFINE_CALLBACK_TRAITS(WRITE, AV);
DEFINE_CALLBACK_TRAITS(WRITE, AVL);
DEFINE_CALLBACK_TRAITS(UPDATE, NONE);
DEFINE_CALLBACK_TRAITS(UPDATE, A);
DEFINE_CALLBACK_TRAITS(UPDATE, AV);
DEFINE_CALLBACK_TRAITS(UPDATE, AVL);

/**
 * Deletes an object holding private data of a thread.
 *
 * @param threadData An object holding private data of a thread.
 */
VOID deleteThreadData(void* threadData)
{
  delete static_cast< ThreadData* >(threadData);
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
 * Deletes a REP instruction execution flag created during thread start.
 *
 * @param repExecutedFlag A flag saying if a REP instruction was executed.
 */
VOID deleteRepExecutedFlag(void* repExecutedFlag)
{
  delete[] static_cast< BOOL* >(repExecutedFlag);
}

/**
 * Gets an object holding private data of a thread.
 *
 * @param tid A number identifying the thread.
 * @return An object holding private data of the thread.
 */
inline
ThreadData* getThreadData(THREADID tid)
{
  return static_cast< ThreadData* >(PIN_GetThreadData(g_threadDataTlsKey, tid));
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
 * Gets a flag saying if a REP instruction was executed.
 *
 * @param tid A number identifying the thread.
 * @return A flag saying if a REP instruction was executed.
 */
inline
BOOL* getRepExecutedFlag(THREADID tid)
{
  return static_cast< BOOL* >(PIN_GetThreadData(g_repExecutedFlagTlsKey, tid));
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
 * @tparam CC A type of concurrent coverage the framework should monitor.
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
template < AccessType AT, CallbackType CT, ConcurrentCoverage CC >
inline
VOID beforeMemoryAccess(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers)
{
  // No Intel instruction have currently more that 2 memory accesses
  assert(memOpIdx < 2);

  if (CC & (CC_SVARS | CC_PREDS))
  { // To identify local variables, we need to know where the stack is situated
    // We monitor SP to find out where the stack can grow (its lowest address)
    if (THREAD_DATA->splow >= GET_REG_VALUE(REG_RSP))
    { // As the memory access might be PUSH, the SP might be a little lower
      THREAD_DATA->splow = GET_REG_VALUE(REG_RSP) - sizeof(ADDRINT);
    }
  }

  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Make sure we have triggered the after callback for the previous access
  ASSERT_MEMORY_ACCESS(memAcc);

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = size;

  if ((CT & (CLBK_AV | CLBK_AVL)) || (CC & CC_SVARS))
  { // Get the variable stored on the accessed address
    getVariable(rtnAddr, insAddr, addr, size, registers, memAcc.var);
  }

  if (CT & CLBK_AVL)
  { // Analysis functions need to get the client lock before accessing locations
    PIN_LockClient();

    // Get the source code location where the memory access originates from
    PIN_GetSourceLocation(insAddr, NULL, &memAcc.loc.line, &memAcc.loc.file);

    // Do not hold the client lock longer that is absolutely necessary
    PIN_UnlockClient();
  }

  if (CC & CC_SVARS)
  { // Notify the shared variables monitor that we are about to access a memory
    if (addr < THREAD_DATA->splow)
    { // Ignore local variables, if the variable has no name, use its address
      g_sVarsMon->beforeVariableAccessed(tid, memAcc.var.name.empty() ?
        VARIABLE(hexstr(addr), memAcc.var.type, memAcc.var.offset) : memAcc.var);
    }
  }

  if (CC & CC_PREDS)
  { // Notify the predecessors monitor that we are about to access a memory
    if (addr < THREAD_DATA->splow)
    { // Ignore local variables, if the variable has no name, use its address
      g_predsMon->beforeVariableAccessed(tid, insAddr, memAcc.var.name.empty() ?
        VARIABLE(hexstr(addr), memAcc.var.type, memAcc.var.offset) : memAcc.var);
    }
  }

  if (CT & CLBK_AVL)
  { // Call all registered TYPE2 callback functions
    typedef callback_traits< AT, CLBK_AVL > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, size, memAcc.var, memAcc.loc);
    }
  }

  if (CT & CLBK_AV)
  { // Call all registered TYPE1 callback functions
    typedef callback_traits< AT, CLBK_AV > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, size, memAcc.var);
    }
  }
}

/**
 * Calls all callback functions registered by a user to be called before
 *   accessing a memory.
 *
 * @note This function is called before a REP instruction accesses a memory.
 *
 * @tparam AT A type of the access.
 * @tparam CT A type of the callback function.
 * @tparam CC A type of concurrent coverage the framework should monitor.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param size A size in bytes of the data accessed.
 * @param memOpIdx An index used to pair before and after memory accesses if
 *   more that one access is performed by a single instruction.
 * @param rtnAddr An address of the routine which accessed the memory.
 * @param insAddr An address of the instruction which accessed the memory.
 * @param registers A structure containing register values.
 * @param isExecuting @em True if the REP instruction will be executed, @em
 *   false otherwise.
 */
template < AccessType AT, CallbackType CT, ConcurrentCoverage CC >
inline
VOID beforeRepMemoryAccess(THREADID tid, ADDRINT addr, UINT32 size,
  UINT32 memOpIdx, ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers,
  BOOL isExecuting)
{
  if (isExecuting)
  { // Call the callback functions only if the instruction will be executed
    beforeMemoryAccess< AT, CT, CC >(tid, addr, size, memOpIdx, rtnAddr,
      insAddr, registers);

    // We need to tell the after callback that the instruction was executed
    getRepExecutedFlag(tid)[memOpIdx] = true;
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
  // No Intel instruction have currently more that 2 memory accesses
  assert(memOpIdx < 2);

  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memOpIdx];

  // Make sure we have triggered the before callback for this access
  assert(memAcc.size != 0);

  if (CT & CLBK_AVL)
  { // Call all registered TYPE2 callback functions
    typedef callback_traits< AT, CLBK_AVL > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.loc);
    }
  }

  if (CT & CLBK_AV)
  { // Call all registered TYPE1 callback functions
    typedef callback_traits< AT, CLBK_AV > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
    }
  }

  // Clear the information about the memory access
  memAcc = MemoryAccess();
}

/**
 * Calls all callback functions registered by a user to be called after
 *   accessing a memory.
 *
 * @note This function is called after a REP instruction accesses a memory.
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
VOID afterRepMemoryAccess(THREADID tid, UINT32 memOpIdx)
{
  if (getRepExecutedFlag(tid)[memOpIdx])
  { // Call the callback functions only if the instruction will be executed
    afterMemoryAccess< AT, CT >(tid, memOpIdx);

    // We do not know if the next REP instruction will be executed
    getRepExecutedFlag(tid)[memOpIdx] = false;
  }
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
  // Allocate memory for storing private data of the starting thread
  PIN_SetThreadData(g_threadDataTlsKey, new ThreadData(), tid);

  // There can only be two simultaneous memory accesses at one time, because no
  // Intel instruction have more that 2 memory accesses, this will suffice then
  PIN_SetThreadData(g_memoryAccessesTlsKey, new MemoryAccess[2], tid);

  // After callback functions do not know if REP instructions were executed and
  // they may perform 2 memory accesses (i.e. there may be 1 or 2 before calls)
  PIN_SetThreadData(g_repExecutedFlagTlsKey, new BOOL[2], tid);
}

/**
 * Setups the synchronisation coverage monitoring.
 *
 * @param settings An object containing the ANaConDA framework's settings.
 */
VOID setupAccessModule(Settings* settings)
{
  g_predsMon = &settings->getCoverageMonitors().preds;
  g_sVarsMon = &settings->getCoverageMonitors().svars;
}

/**
 * Gets a section of the memory access instrumentation settings which contains
 *   the settings for a specific type of memory access.
 *
 * @tparam AT A type of the access.
 *
 * @param mais A structure containing memory access instrumentation settings.
 * @return A section in the structure containing settings for a specific type
 *   of memory access.
 */
template< AccessType AT >
inline
InstrumentationSettings& section(MemoryAccessInstrumentationSettings& mais)
{
  switch (AT)
  { // Return the section of the MAIS structure for the specified memory access
    case READ: // Read access
      return mais.reads;
    case WRITE: // Write access
      return mais.writes;
    case UPDATE: // Atomic update access
      return mais.updates;
    default: // The execution should never reach this part
      assert(false);
  }
}

/**
 * Setups callback functions which should be called before memory accesses.
 *
 * @note This function is called recursively until appropriate set of callback
 *   functions is found. The goal is to find a set of functions which extract
 *   only the information needed by the analyser, nothing less, nothing more.
 *
 * @tparam AT A type of the access.
 * @tparam CT A type of the callback function.
 *
 * @param mais A structure containing memory access instrumentation settings.
 */
template< AccessType AT, CallbackType CT >
inline
VOID setupBeforeCallbackFunction(MemoryAccessInstrumentationSettings& mais)
{
  if (!callback_traits< AT, CT >::before.empty())
  { // This set of functions give us all, but minimal, info the analysers need
    if (mais.predecessors)
    { // Monitor predecessors
      section< AT >(mais).beforeCallback = mais.sharedVars ?
        (AFUNPTR)beforeMemoryAccess< AT, CT, (ConcurrentCoverage)(CC_PREDS | CC_SVARS) >:
        (AFUNPTR)beforeMemoryAccess< AT, CT, CC_PREDS >;
      section< AT >(mais).beforeRepCallback = mais.sharedVars ?
        (AFUNPTR)beforeRepMemoryAccess< AT, CT, (ConcurrentCoverage)(CC_PREDS | CC_SVARS) >:
        (AFUNPTR)beforeRepMemoryAccess< AT, CT, CC_PREDS >;
    }
    else
    { // Do not monitor predecessors
      section< AT >(mais).beforeCallback = mais.sharedVars ?
        (AFUNPTR)beforeMemoryAccess< AT, CT, CC_SVARS >:
        (AFUNPTR)beforeMemoryAccess< AT, CT, CC_NONE >;
      section< AT >(mais).beforeRepCallback = mais.sharedVars ?
        (AFUNPTR)beforeRepMemoryAccess< AT, CT, CC_SVARS >:
        (AFUNPTR)beforeRepMemoryAccess< AT, CT, CC_NONE >;
    }
    section< AT >(mais).beforeCallbackType = CT;
  }
  else if (CT != 0) // Try another set of callback functions if any set remains
    setupBeforeCallbackFunction< AT, static_cast< CallbackType >(CT / 2) >(mais);
}

/**
 * Setups callback functions which should be called after memory accesses.
 *
 * @note This function is called recursively until appropriate set of callback
 *   functions is found. The goal is to find a set of functions which extract
 *   only the information needed by the analyser, nothing less, nothing more.
 *
 * @tparam AT A type of the access.
 * @tparam CT A type of the callback function.
 *
 * @param mais A structure containing memory access instrumentation settings.
 */
template< AccessType AT, CallbackType CT >
inline
VOID setupAfterCallbackFunction(MemoryAccessInstrumentationSettings& mais)
{
  if (!callback_traits< AT, CT >::after.empty())
  { // This set of functions give us all, but minimal, info the analysers need
    section< AT >(mais).afterCallback = (AFUNPTR)
      afterMemoryAccess< AT, CT >;
    section< AT >(mais).afterRepCallback = (AFUNPTR)
      afterRepMemoryAccess< AT, CT >;
    section< AT >(mais).afterCallbackType = CT;
  }
  else if (CT != 0) // Try another set of callback functions if any set remains
    setupAfterCallbackFunction< AT, static_cast< CallbackType >(CT / 2) >(mais);
}

/**
 * Setups memory access callback functions and their types.
 *
 * @param mais An object containing memory access instrumentation settings.
 */
VOID setupMemoryAccessSettings(MemoryAccessInstrumentationSettings& mais)
{
  // Setup a callback function which will be called before reads
  setupBeforeCallbackFunction< READ, CLBK_AVL >(mais);

  // Setup a callback function which will be called before writes
  setupBeforeCallbackFunction< WRITE, CLBK_AVL >(mais);

  // Setup a callback function which will be called before updates
  setupBeforeCallbackFunction< UPDATE, CLBK_AVL >(mais);

  // Setup a callback function which will be called after reads
  setupAfterCallbackFunction< READ, CLBK_AVL >(mais);

  // Setup a callback function which will be called after writes
  setupAfterCallbackFunction< WRITE, CLBK_AVL >(mais);

  // Setup a callback function which will be called after updates
  setupAfterCallbackFunction< UPDATE, CLBK_AVL >(mais);

  // If no callback is registered, there is no need to instrument the accesses
  mais.instrument
    = (bool)mais.reads.beforeCallback
    | (bool)mais.reads.afterCallback
    | (bool)mais.writes.beforeCallback
    | (bool)mais.writes.afterCallback
    | (bool)mais.updates.beforeCallback
    | (bool)mais.updates.afterCallback;
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREADAVFUNPTR callback)
{
  callback_traits< READ, CLBK_AV >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREADAVLFUNPTR callback)
{
  callback_traits< READ, CLBK_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVFUNPTR callback)
{
  callback_traits< WRITE, CLBK_AV >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVLFUNPTR callback)
{
  callback_traits< WRITE, CLBK_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called before atomically
 *   updating a memory.
 */
VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVFUNPTR callback)
{
  callback_traits< UPDATE, CLBK_AV >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called before atomically
 *   updating a memory.
 */
VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVLFUNPTR callback)
{
  callback_traits< UPDATE, CLBK_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREADAVFUNPTR callback)
{
  callback_traits< READ, CLBK_AV >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREADAVLFUNPTR callback)
{
  callback_traits< READ, CLBK_AVL >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITEAVFUNPTR callback)
{
  callback_traits< WRITE, CLBK_AV >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITEAVLFUNPTR callback)
{
  callback_traits< WRITE, CLBK_AVL >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called after atomically
 *   updating a memory.
 */
VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVFUNPTR callback)
{
  callback_traits< UPDATE, CLBK_AV >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called after atomically
 *   updating a memory.
 */
VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVLFUNPTR callback)
{
  callback_traits< UPDATE, CLBK_AVL >::after.push_back(callback);
}

/** End of file access.cpp **/
