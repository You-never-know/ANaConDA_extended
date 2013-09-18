/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-09-18
 * @version   0.8.7
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

#include "../defs.h"
#include "../settings.h"
#include "../types.h"

/**
 * @brief An enumeration describing the information which might be requested by
 *   the callback functions registered to be called when a memory is accessed.
 */
typedef enum AccessInfo_e
{
  /**
   * @brief No information.
   */
  AI_NONE        = 0x0,
  /**
   * @brief Basic information about a memory access. This information includes
   *   the address and the amount of bytes accessed.
   */
  AI_ACCESS      = 0x1,
  /**
   * @brief Information about a variable accessed. This information includes
   *   the name of the variable and its data type.
   */
  AI_VARIABLE    = 0x2,
  /**
   * @brief Information about a location which performed a memory access. This
   *   information includes the name of the file in which the location is
   *   situated together with the line number identifying the location.
   */
  AI_LOCATION    = 0x4,
  /**
   * @brief Information about an instruction which performed a memory access.
   *   This information includes the address of this instruction.
   */
  AI_INSTRUCTION = 0x8,
  /**
   * @brief Information about a locality of a memory access. This information
   *   includes a flag determining if the memory accessed lies in the part of
   *   the memory reserved for a stack. This flag can be used to distinguish
   *   between local and global memory accesses.
   */
  AI_ON_STACK    = 0xF
} AccessInfo;

/**
 * @brief An enumeration describing the types of various callback functions.
 */
typedef enum CallbackType_e
{
  /**
   * @brief An invalid callback function.
   */
  CT_INVALID = AI_NONE,
  /**
   * @brief A callback function providing address of the memory accessed.
   */
  CT_A = AI_ACCESS,
  /**
   * @brief A callback function providing address of the memory accessed
   *   and information about the variable residing at this address.
   */
  CT_AV = AI_ACCESS | AI_VARIABLE,
  /**
   * @brief A callback function providing address of the memory accessed,
   *   information about the variable residing at this address and location
   *   in the source code which performed the memory access.
   */
  CT_AVL = AI_ACCESS | AI_VARIABLE | AI_LOCATION
} CallbackType;

/**
 * @brief A structure containing information needed to instrument memory
 *   accesses.
 */
typedef struct MemoryAccessInstrumentationSettings_s
{
  AFUNPTR beforeAccess; //!< A function called before a memory access.
  /**
   * @brief A function called before a repeatable memory access (usually caused
   *   by an instruction with a REP prefix).
   */
  AFUNPTR beforeRepAccess;
  AFUNPTR afterAccess; //!< A function called after a memory access.
  /**
   * @brief A function called after a repeatable memory access (usually caused
   *   by an instruction with a REP prefix).
   */
  AFUNPTR afterRepAccess;
  /**
   * @brief Information needed by the functions called before a memory access.
   */
  AccessInfo beforeAccessInfo;
  /**
   * @brief Information needed by the functions called after a memory access.
   */
  AccessInfo afterAccessInfo;
  /**
   * @brief A structure containing detailed information about a noise which
   *   should be inserted before a memory access.
   */
  NoiseSettings* noise;

  /**
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   */
  MemoryAccessInstrumentationSettings_s() : beforeAccess(NULL),
    beforeRepAccess(NULL), afterAccess(NULL), afterRepAccess(NULL),
    beforeAccessInfo(AI_NONE), afterAccessInfo(AI_NONE), noise(NULL) {}

  /**
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   *
   * @param ns A structure containing detailed information about a noise which
   *   should be inserted before a memory access.
   */
  MemoryAccessInstrumentationSettings_s(NoiseSettings* ns) : beforeAccess(NULL),
    beforeRepAccess(NULL), afterAccess(NULL), afterRepAccess(NULL),
    beforeAccessInfo(AI_NONE), afterAccessInfo(AI_NONE), noise(ns) {}
} MemoryAccessInstrumentationSettings;

/**
 * @brief A structure containing information needed to monitor memory accesses.
 */
typedef struct MemoryAccessSettings_s
{
  /**
   * @brief A structure describing how to instrument memory accesses reading
   *   from a memory.
   */
  MemoryAccessInstrumentationSettings reads;
  /**
   * @brief A structure describing how to instrument memory accesses writing to
   *   a memory.
   */
  MemoryAccessInstrumentationSettings writes;
  /**
   * @brief A structure describing how to instrument memory accesses atomically
   *   updating a memory.
   */
  MemoryAccessInstrumentationSettings updates;
  /**
   * @brief A flag determining if instrumenting memory accesses is necessary,
   *   i.e., set to @em true if at least one callback function is registered
   *   and to @em false if no callback function is registered.
   */
  bool instrument;
  /**
   * @brief A flag determining if monitoring shared variables is requested.
   */
  bool sharedVars;
  /**
   * @brief A flag determining if monitoring predecessors is requested.
   */
  bool predecessors;

  /**
   * Constructs a MemoryAccessSettings_s object.
   */
  MemoryAccessSettings_s() : reads(), writes(), updates(),
    instrument(false), sharedVars(false), predecessors(false) {}

  /**
   * Constructs a MemoryAccessSettings_s object.
   *
   * @param s An object containing the ANaConDA framework's settings.
   */
  MemoryAccessSettings_s(Settings* s) : reads(s->getReadNoise()),
   writes(s->getWriteNoise()), updates(s->getUpdateNoise()), instrument(false),
   sharedVars(s->get< bool >("coverage.sharedvars")),
   predecessors(s->get< bool >("coverage.predecessors")) {}
} MemoryAccessSettings;

// Definitions of analysis functions (callback functions called by PIN)
VOID initMemoryAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

// Definitions of helper functions
VOID setupAccessModule(Settings* settings);
VOID setupMemoryAccessSettings(MemoryAccessSettings& mas);

// Definitions of callback functions
typedef VOID (*MEMREADAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMREADAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMREADAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMWRITEAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMWRITEAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMWRITEAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMUPDATEAFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size);
typedef VOID (*MEMUPDATEAVFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMUPDATEAVLFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);

// Definitions of functions for registering callback functions
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREADAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATEAVLFUNPTR callback);

API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREADAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITEAVLFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVFUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATEAVLFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
