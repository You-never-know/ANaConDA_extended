/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing implementation of access-related callback functions.
 *
 * A file containing implementation of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2019-06-04
 * @version   0.10
 */

#include "access.h"

#include "libdie-wrapper/pin_die.h"

#include "../anaconda.h"

#include "../monitors/preds.hpp"
#include "../monitors/svars.hpp"

#include "../utils/ctops.hpp"

/**
 * @brief A structure containing information about a memory access.
 */
typedef struct MemoryAccess_s
{
  ADDRINT addr; //!< An accessed address.
  UINT32 size; //!< A size in bytes accessed.
  VARIABLE var; //!< A variable accessed.
  LOCATION loc; //!< A source code location where the access originates from.
  ADDRINT ins; //!< An address of the instruction which performed the access.
#ifdef DEBUG_MEMORY_ACCESSES
  ADDRINT rtn; //!< An address of the routine which performed the access.
#endif

  /**
   * Constructs a MemoryAccess_s object.
   */
#ifdef DEBUG_MEMORY_ACCESSES
  MemoryAccess_s() : addr(0), size(0), var(), loc(), ins(0), rtn(0) {}
#else
  MemoryAccess_s() : addr(0), size(0), var(), loc(), ins(0) {}
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
#define IS_REGISTERED(ct) ctops::contains< CallbackType, ct, Callbacks... >()

// Temporary fix until CODAN starts supporting pointers to variadic templates
#if defined(ECLIPSE_CDT_ENABLE_CODAN_FIXES)
  #define EXPAND(args) args
#else
  #define EXPAND(args) args...
#endif

// Definitions of callback functions needed to instantiate traits for CT_INVALID
typedef VOID (*MEMREADINVALIDFUNPTR)();
typedef VOID (*MEMWRITEINVALIDFUNPTR)();
typedef VOID (*MEMUPDATEINVALIDFUNPTR)();

// Declarations of static functions (usable only within this module)
static VOID deleteThreadData(void* threadData);
static VOID deleteMemoryAccesses(void* memoryAccesses);
static VOID deleteRepExecutedFlag(void* repExecutedFlag);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_threadDataTlsKey = PIN_CreateThreadDataKey(deleteThreadData);
  TLS_KEY g_memoryAccessesTlsKey = PIN_CreateThreadDataKey(deleteMemoryAccesses);
  TLS_KEY g_repExecutedFlagTlsKey = PIN_CreateThreadDataKey(deleteRepExecutedFlag);
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
 *   enumeration, but only the second part, e.g., the name from CT_<name>).
 */
#define DEFINE_CALLBACK_TRAITS(access, callback) \
  template<> \
  struct callback_traits< access, CT_##callback > \
  { \
    typedef MEM##access##callback##FUNPTR fun_ptr_type; \
    typedef std::vector< fun_ptr_type > container_type; \
    static container_type before; \
    static container_type after; \
  }; \
  callback_traits< access, CT_##callback >::container_type \
    callback_traits< access, CT_##callback >::before; \
  callback_traits< access, CT_##callback >::container_type \
    callback_traits< access, CT_##callback >::after

// Define traits information for the known types of callback functions
DEFINE_CALLBACK_TRAITS(READ, INVALID);
DEFINE_CALLBACK_TRAITS(READ, A);
DEFINE_CALLBACK_TRAITS(READ, AV);
DEFINE_CALLBACK_TRAITS(READ, AVL);
DEFINE_CALLBACK_TRAITS(READ, AVO);
DEFINE_CALLBACK_TRAITS(READ, AVIO);
DEFINE_CALLBACK_TRAITS(WRITE, INVALID);
DEFINE_CALLBACK_TRAITS(WRITE, A);
DEFINE_CALLBACK_TRAITS(WRITE, AV);
DEFINE_CALLBACK_TRAITS(WRITE, AVL);
DEFINE_CALLBACK_TRAITS(WRITE, AVO);
DEFINE_CALLBACK_TRAITS(WRITE, AVIO);
DEFINE_CALLBACK_TRAITS(UPDATE, INVALID);
DEFINE_CALLBACK_TRAITS(UPDATE, A);
DEFINE_CALLBACK_TRAITS(UPDATE, AV);
DEFINE_CALLBACK_TRAITS(UPDATE, AVL);
DEFINE_CALLBACK_TRAITS(UPDATE, AVO);
DEFINE_CALLBACK_TRAITS(UPDATE, AVIO);

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
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam Callbacks A list of types of callback functions registered.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param registers A structure containing register values.
 * @param memAccInfo A structure containing static (non-changing) information
 *   about the access.
 */
template < AccessType AT, AccessInfo AI, CallbackType... Callbacks >
inline
VOID PIN_FAST_ANALYSIS_CALL beforeMemoryAccess(THREADID tid, ADDRINT addr,
  CONTEXT* registers, MemoryAccessInfo* memAccInfo)
{
  // No Intel instruction have currently more that 2 memory accesses
  assert(memAccInfo->index < 2);

  if (AI & AI_ON_STACK)
  { // To identify local variables, we need to know where the stack is situated
    // We monitor SP to find out where the stack can grow (its lowest address)
    if (THREAD_DATA->splow >= GET_REG_VALUE(REG_STACK_PTR))
    { // As the memory access might be PUSH, the SP might be a little lower
      THREAD_DATA->splow = GET_REG_VALUE(REG_STACK_PTR) - sizeof(ADDRINT);
    }
  }

  // Get the object to which the info about the memory access should be stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memAccInfo->index];

  // Make sure we have triggered the after callback for the previous access
  ASSERT_MEMORY_ACCESS(memAcc);

  // Accessed address and size is not available after the memory access
  memAcc.addr = addr;
  memAcc.size = memAccInfo->size;

  if (AI & AI_VARIABLE)
  { // Get the variable stored on the accessed address
    getVariable(memAccInfo->instruction->rtnAddress,
      memAccInfo->instruction->address, addr, memAccInfo->size, registers,
      memAcc.var);
  }

  if (AI & AI_LOCATION)
  { // Analysis functions need to get the client lock before accessing locations
    PIN_LockClient();

    // Get the source code location where the memory access originates from
    PIN_GetSourceLocation(memAccInfo->instruction->address, NULL,
      &memAcc.loc.line, &memAcc.loc.file);

    // Do not hold the client lock longer that is absolutely necessary
    PIN_UnlockClient();
  }

  if (AI & AI_INSTRUCTION)
  { // Instruction address is also not available after the memory access
    memAcc.ins = memAccInfo->instruction->address;
  }

  if (IS_REGISTERED(CT_AVL))
  { // Call all registered AVL-type callback functions
    typedef callback_traits< AT, CT_AVL > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, memAccInfo->size, memAcc.var, memAcc.loc);
    }
  }

  if (IS_REGISTERED(CT_AV))
  { // Call all registered AV-type callback functions
    typedef callback_traits< AT, CT_AV > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, memAccInfo->size, memAcc.var);
    }
  }

  if (IS_REGISTERED(CT_AVO))
  { // Call all registered AVO-type callback functions
    typedef callback_traits< AT, CT_AVO > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, memAccInfo->size, memAcc.var, addr >= THREAD_DATA->splow);
    }
  }

  if (IS_REGISTERED(CT_AVIO))
  { // Call all registered AVO-type callback functions
    typedef callback_traits< AT, CT_AVIO > Traits;

    for (typename Traits::container_type::iterator it = Traits::before.begin();
      it != Traits::before.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, addr, memAccInfo->size, memAcc.var,
        memAccInfo->instruction->address, addr >= THREAD_DATA->splow);
    }
  }
}

/**
 * Calls all callback functions registered by a user to be called before
 *   accessing a memory.
 *
 * @note This function is called before a REP instruction accesses a memory.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam Callbacks A list of types of callback functions registered.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param addr An address of the data accessed.
 * @param registers A structure containing register values.
 * @param isExecuting @em True if the REP instruction will be executed, @em
 *   false otherwise.
 * @param memAccInfo A structure containing static (non-changing) information
 *   about the access.
 */
template < AccessType AT, AccessInfo AI, CallbackType... Callbacks >
inline
VOID PIN_FAST_ANALYSIS_CALL beforeRepMemoryAccess(THREADID tid, ADDRINT addr,
  CONTEXT* registers, BOOL isExecuting, MemoryAccessInfo* memAccInfo)
{
  if (isExecuting)
  { // Call the callback functions only if the instruction will be executed
    beforeMemoryAccess< AT, AI, Callbacks... >(tid, addr, registers, memAccInfo);

    // We need to tell the after callback that the instruction was executed
    getRepExecutedFlag(tid)[memAccInfo->index] = true;
  }
}

/**
 * Calls all callback functions registered by a user to be called after
 *   accessing a memory.
 *
 * @note This function is called after an instruction accesses a memory.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam Callbacks A list of types of callback functions registered.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param memAccInfo A structure containing static (non-changing) information
 *   about the access.
 */
template < AccessType AT, AccessInfo AI, CallbackType... Callbacks >
inline
VOID PIN_FAST_ANALYSIS_CALL afterMemoryAccess(THREADID tid,
  MemoryAccessInfo* memAccInfo)
{
  // No Intel instruction have currently more that 2 memory accesses
  assert(memAccInfo->index < 2);

  // Get the object in which the info about the memory access is stored
  MemoryAccess& memAcc = getLastMemoryAccesses(tid)[memAccInfo->index];

  // Make sure we have triggered the before callback for this access
  assert(memAcc.size != 0);

  if (IS_REGISTERED(CT_AVL))
  { // Call all registered AVL-type callback functions
    typedef callback_traits< AT, CT_AVL > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.loc);
    }
  }

  if (IS_REGISTERED(CT_AV))
  { // Call all registered AV-type callback functions
    typedef callback_traits< AT, CT_AV > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var);
    }
  }

  if (IS_REGISTERED(CT_AVO))
  { // Call all registered AV-type callback functions
    typedef callback_traits< AT, CT_AVO > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var,
        memAcc.addr >= THREAD_DATA->splow);
    }
  }

  if (IS_REGISTERED(CT_AVIO))
  { // Call all registered AV-type callback functions
    typedef callback_traits< AT, CT_AVIO > Traits;

    for (typename Traits::container_type::iterator it = Traits::after.begin();
      it != Traits::after.end(); it++)
    { // Call all callback functions registered by the user (used analyser)
      (*it)(tid, memAcc.addr, memAcc.size, memAcc.var, memAcc.ins,
        memAcc.addr >= THREAD_DATA->splow);
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
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam Callbacks A list of types of callback functions registered.
 *
 * @param tid A number identifying the thread which performed the access.
 * @param memAccInfo A structure containing static (non-changing) information
 *   about the access.
 */
template < AccessType AT, AccessInfo AI, CallbackType... Callbacks >
inline
VOID PIN_FAST_ANALYSIS_CALL afterRepMemoryAccess(THREADID tid,
  MemoryAccessInfo* memAccInfo)
{
  if (getRepExecutedFlag(tid)[memAccInfo->index])
  { // Call the callback functions only if the instruction will be executed
    afterMemoryAccess< AT, AI, Callbacks... >(tid, memAccInfo);

    // We do not know if the next REP instruction will be executed
    getRepExecutedFlag(tid)[memAccInfo->index] = false;
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
}

namespace detail
{ // Implementation details, never use directly!

/**
 * Gets a section of the memory access instrumentation settings which contains
 *   the settings for a specific type of memory access.
 *
 * @tparam AT A type of the access.
 *
 * @param mas A structure containing memory access instrumentation settings.
 * @return A section in the structure containing settings for a specific type
 *   of memory access.
 */
template< AccessType AT >
inline
MemoryAccessInstrumentationSettings& section(MemoryAccessSettings& mas)
{
  switch (AT)
  { // Return the section of the MAS structure for the specified memory access
    case READ: // Read access
      return mas.reads;
    case WRITE: // Write access
      return mas.writes;
    case UPDATE: // Atomic update access
      return mas.updates;
    default: // The execution should never reach this part
      assert(false);
      break;
  }
}

/**
 * Terminates the program with an assertion error.
 *
 * @note This function should never be called, but it is needed to properly
 *   instantiate some of the template functions in this module.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 *
 * @param mas A structure containing memory access instrumentation settings.
 */
template < AccessType AT, AccessInfo AI >
inline
VOID setupBeforeCallbacks(MemoryAccessSettings& mas) { assert(false); }

/**
 * Setups functions which should be called before memory accesses.
 *
 * @note The goal of this function is to find a set of functions which extract
 *   only the information needed by the analysers, nothing less, nothing more.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam CT The currently processed type of callback functions.
 * @tparam Args A list of types of callback functions yet to be processed.
 *
 * @warning The list containing the types of callback functions which should be
 *   processed, the (CT, Args...) list, must have the @c CT_INVALID type as its
 *   last item. The function uses it to determined that it reached the end of
 *   the list as it put types of callback function which need to be activated
 *   after the end of the list.
 *
 * @param mas A structure to which the information about the functions which
 *   should be called before memory accesses will be stored.
 */
template < AccessType AT, AccessInfo AI, CallbackType CT, CallbackType... Args >
inline
VOID setupBeforeCallbacks(MemoryAccessSettings& mas)
{
  if (CT == CT_INVALID)
  { // We have processed all types of callback functions given, now instantiate
    // a set of functions able to get all the information needed by the enabled
    // types of callback functions, but not more, the enabled types of callback
    // functions are stored in the Args template parameter now
    section< AT >(mas).beforeAccess = (AFUNPTR)
      beforeMemoryAccess< AT, AI, EXPAND(Args) >;
    section< AT >(mas).beforeRepAccess = (AFUNPTR)
      beforeRepMemoryAccess< AT, AI, EXPAND(Args) >;
    section< AT >(mas).beforeAccessInfo = AI;

    return; // Setup is complete
  }

  if (!callback_traits< AT, CT >::before.empty())
  { // Some callback functions of the currently processed type are registered,
    // remember that we need to enable this type of callback functions in the
    // functions we will be instantiating and update AccessInfo to know which
    // kind of information we will need to extract for them
    setupBeforeCallbacks< AT, (AccessInfo)(AI | CT), Args..., CT >(mas);
  }
  else
  { // Else continue processing the remaining types of callback functions
    setupBeforeCallbacks< AT, AI, Args... >(mas);
  }
}

/**
 * Terminates the program with an assertion error.
 *
 * @note This function should never be called, but it is needed to properly
 *   instantiate some of the template functions in this module.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 *
 * @param mas A structure containing memory access instrumentation settings.
 */
template < AccessType AT, AccessInfo AI >
inline
VOID setupAfterCallbacks(MemoryAccessSettings& mas) { assert(false); }

/**
 * Setups functions which should be called after memory accesses.
 *
 * @note The goal of this function is to find a set of functions which extract
 *   only the information needed by the analysers, nothing less, nothing more.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam AI Information needed by the registered callback functions.
 * @tparam CT The currently processed type of callback functions.
 * @tparam Args A list of types of callback functions yet to be processed.
 *
 * @warning The list containing the types of callback functions which should be
 *   processed, the (CT, Args...) list, must have the @c CT_INVALID type as its
 *   last item. The function uses it to determined that it reached the end of
 *   the list as it put types of callback function which need to be activated
 *   after the end of the list.
 *
 * @param mas A structure to which the information about the functions which
 *   should be called after memory accesses will be stored.
 */
template < AccessType AT, AccessInfo AI, CallbackType CT, CallbackType... Args >
inline
VOID setupAfterCallbacks(MemoryAccessSettings& mas)
{
  if (CT == CT_INVALID)
  { // We have processed all types of callback functions given, now instantiate
    // a set of functions able to get all the information needed by the enabled
    // types of callback functions, but not more, the enabled types of callback
    // functions are stored in the Args template parameter now
    section< AT >(mas).afterAccess = (AFUNPTR)
      afterMemoryAccess< AT, AI, EXPAND(Args) >;
    section< AT >(mas).afterRepAccess = (AFUNPTR)
      afterRepMemoryAccess< AT, AI, EXPAND(Args) >;
    section< AT >(mas).afterAccessInfo = AI;

    return; // Setup is complete
  }

  if (!callback_traits< AT, CT >::after.empty())
  { // Some callback functions of the currently processed type are registered,
    // remember that we need to enable this type of callback functions in the
    // functions we will be instantiating and update AccessInfo to know which
    // kind of information we will need to extract for them
    setupAfterCallbacks< AT, (AccessInfo)(AI | CT), Args..., CT >(mas);
  }
  else
  { // Else continue processing the remaining types of callback functions
    setupAfterCallbacks< AT, AI, Args... >(mas);
  }
}

} // namespace detail

/**
 * Setups functions which should be called before memory accesses.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam Supported A list of types of callback functions supported by the
 *   framework.
 *
 * @param mas A structure to which the information about the functions which
 *   should be called before memory accesses will be stored.
 */
template < AccessType AT, CallbackType... Supported >
inline
VOID setupBeforeCallbacks(MemoryAccessSettings& mas)
{
  detail::setupBeforeCallbacks< AT, AI_NONE, Supported..., CT_INVALID >(mas);
}

/**
 * Setups functions which should be called after memory accesses.
 *
 * @tparam AT A type of the access (read, write, atomic update, etc.).
 * @tparam Supported A list of types of callback functions supported by the
 *   framework.
 *
 * @param mas A structure to which the information about the functions which
 *   should be called after memory accesses will be stored.
 */
template < AccessType AT, CallbackType... Supported >
inline
VOID setupAfterCallbacks(MemoryAccessSettings& mas)
{
  detail::setupAfterCallbacks< AT, AI_NONE, Supported..., CT_INVALID >(mas);
}

/**
 * Setups memory access callback functions and their types.
 *
 * @param mas An object containing memory access instrumentation settings.
 */
VOID setupMemoryAccessSettings(MemoryAccessSettings& mas)
{
  // Setup callback functions which will be called before reads
  setupBeforeCallbacks< READ, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // Setup callback functions which will be called before writes
  setupBeforeCallbacks< WRITE, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // Setup callback functions which will be called before updates
  setupBeforeCallbacks< UPDATE, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // Setup callback functions which will be called after reads
  setupAfterCallbacks< READ, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // Setup callback functions which will be called after writes
  setupAfterCallbacks< WRITE, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // Setup callback functions which will be called after updates
  setupAfterCallbacks< UPDATE, CT_AVIO, CT_AVO, CT_AVL, CT_AV, CT_A >(mas);

  // If no information is needed, there is no need to instrument the accesses
  mas.instrument = mas.reads.beforeAccessInfo | mas.reads.afterAccessInfo
                 | mas.writes.beforeAccessInfo | mas.writes.afterAccessInfo
                 | mas.updates.beforeAccessInfo | mas.updates.afterAccessInfo;
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
  callback_traits< READ, CT_AV >::before.push_back(callback);
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
  callback_traits< READ, CT_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREADAVOFUNPTR callback)
{
  callback_traits< READ, CT_AVO >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before reading from a
 *   memory.
 *
 * @param callback A callback function which should be called before reading
 *   from a memory.
 */
VOID ACCESS_BeforeMemoryRead(MEMREADAVIOFUNPTR callback)
{
  callback_traits< READ, CT_AVIO >::before.push_back(callback);
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
  callback_traits< WRITE, CT_AV >::before.push_back(callback);
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
  callback_traits< WRITE, CT_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVOFUNPTR callback)
{
  callback_traits< WRITE, CT_AVO >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before writing to a
 *   memory.
 *
 * @param callback A callback function which should be called before writing to
 *   a memory.
 */
VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVIOFUNPTR callback)
{
  callback_traits< WRITE, CT_AVIO >::before.push_back(callback);
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
  callback_traits< UPDATE, CT_AV >::before.push_back(callback);
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
  callback_traits< UPDATE, CT_AVL >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called before atomically
 *   updating a memory.
 */
VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVOFUNPTR callback)
{
  callback_traits< UPDATE, CT_AVO >::before.push_back(callback);
}

/**
 * Registers a callback function which will be called before atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called before atomically
 *   updating a memory.
 */
VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVIOFUNPTR callback)
{
  callback_traits< UPDATE, CT_AVIO >::before.push_back(callback);
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
  callback_traits< READ, CT_AV >::after.push_back(callback);
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
  callback_traits< READ, CT_AVL >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREADAVOFUNPTR callback)
{
  callback_traits< READ, CT_AVO >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after reading from a
 *   memory.
 *
 * @param callback A callback function which should be called after reading
 *   from a memory.
 */
VOID ACCESS_AfterMemoryRead(MEMREADAVIOFUNPTR callback)
{
  callback_traits< READ, CT_AVIO >::after.push_back(callback);
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
  callback_traits< WRITE, CT_AV >::after.push_back(callback);
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
  callback_traits< WRITE, CT_AVL >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITEAVOFUNPTR callback)
{
  callback_traits< WRITE, CT_AVO >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after writing to a
 *   memory.
 *
 * @param callback A callback function which should be called after writing to
 *   a memory.
 */
VOID ACCESS_AfterMemoryWrite(MEMWRITEAVIOFUNPTR callback)
{
  callback_traits< WRITE, CT_AVIO >::after.push_back(callback);
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
  callback_traits< UPDATE, CT_AV >::after.push_back(callback);
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
  callback_traits< UPDATE, CT_AVL >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called after atomically
 *   updating a memory.
 */
VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVOFUNPTR callback)
{
  callback_traits< UPDATE, CT_AVO >::after.push_back(callback);
}

/**
 * Registers a callback function which will be called after atomically updating
 *   a memory.
 *
 * @param callback A callback function which should be called after atomically
 *   updating a memory.
 */
VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVIOFUNPTR callback)
{
  callback_traits< UPDATE, CT_AVIO >::after.push_back(callback);
}

/**
 * Gets a location in the source code corresponding to an instruction accessing
 *   a memory.
 *
 * @param ins An address of an instruction performing a memory access.
 * @param location A source code location corresponding to the instruction
 *   accessing a memory.
 */
VOID ACCESS_GetLocation(ADDRINT ins, LOCATION& location)
{ // Analysis functions need to get the client lock before accessing locations
  PIN_LockClient();

  // Get the source code location where the memory access originates from
  PIN_GetSourceLocation(ins, NULL, &location.line, &location.file);

  // Do not hold the client lock longer that is absolutely necessary
  PIN_UnlockClient();
}

/** End of file access.cpp **/
