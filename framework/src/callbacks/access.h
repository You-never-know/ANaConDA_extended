/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-09-05
 * @version   0.8.3
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

#include "../defs.h"
#include "../settings.h"
#include "../types.h"

/**
 * @brief An enumeration describing the information which might be requested by
 *   callback functions.
 */
typedef enum AccessInfo_e
{
  AI_NONE = 0x0, //!< No information.
  AI_A    = 0x1, //!< An address of the memory accessed.
  AI_V    = 0x2, //!< A variable accessed.
  AI_L    = 0x4, //!< A location which performed the access.
  AI_I    = 0x8, //!< An address of the instruction which performed the access.
  AI_O    = 0xF  //!< A flag determining if the accessed variable is local.
} AccessInfo;

/**
 * @brief An enumeration describing the types of various callback functions.
 */
typedef enum CallbackType_e
{
  /**
   * @brief An invalid callback function.
   */
  CT_NONE = 0x0,
  /**
   * @brief A callback function providing address of the memory accessed.
   */
  CT_A    = 0x1,
  /**
   * @brief A callback function providing address of the memory accessed
   *   and information about the variable residing at this address.
   */
  CT_AV   = 0x2,
  /**
   * @brief A callback function providing address of the memory accessed,
   *   information about the variable residing at this address and location
   *   in the source code which performed the memory access.
   */
  CT_AVL  = 0x4
} CallbackType;

/**
 * @brief A structure containing instrumentation settings.
 */
typedef struct InstrumentationSettings_s
{
  AFUNPTR beforeCallback; //!< A function called before an instrumented object.
  /**
   * @brief A function called before an instrumented object with a REP prefix.
   */
  AFUNPTR beforeRepCallback;
  AFUNPTR afterCallback; //!< A function called after an instrumented object.
  /**
   * @brief A function called after an instrumented object with a REP prefix.
   */
  AFUNPTR afterRepCallback;
  /**
   * @brief A type of the function called before an instrumented object.
   */
  CallbackType beforeCallbackType;
  /**
   * @brief A type of the function called after an instrumented object.
   */
  CallbackType afterCallbackType;
  /**
   * @brief A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  NoiseSettings* noise;

  /**
   * Constructs an InstrumentationSettings_s object.
   */
  InstrumentationSettings_s() : beforeCallback(NULL), beforeRepCallback(NULL),
    afterCallback(NULL), afterRepCallback(NULL), beforeCallbackType(CT_NONE),
    afterCallbackType(CT_NONE), noise(NULL) {}

  /**
   * Constructs an InstrumentationSettings_s object.
   *
   * @param ns A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  InstrumentationSettings_s(NoiseSettings* ns) : beforeCallback(NULL),
    beforeRepCallback(NULL), afterCallback(NULL), afterRepCallback(NULL),
    beforeCallbackType(CT_NONE), afterCallbackType(CT_NONE), noise(ns) {}
} InstrumentationSettings;

/**
 * @brief A structure containing memory access instrumentation settings.
 */
typedef struct MemoryAccessInstrumentationSettings_s
{
  /**
   * @brief A structure describing how to instrument instructions reading from
   *   a memory.
   */
  InstrumentationSettings reads;
  /**
   * @brief A structure describing how to instrument instructions writing to
   *   a memory.
   */
  InstrumentationSettings writes;
  /**
   * @brief A structure describing how to instrument instructions atomically
   *   updating a memory.
   */
  InstrumentationSettings updates;
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
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   */
  MemoryAccessInstrumentationSettings_s() : reads(), writes(), updates(),
    instrument(false), sharedVars(false), predecessors(false) {}

  /**
   * Constructs a MemoryAccessSettings_s object.
   *
   * @param s An object containing the ANaConDA framework's settings.
   */
  MemoryAccessInstrumentationSettings_s(Settings* s) : reads(s->getReadNoise()),
   writes(s->getWriteNoise()), updates(s->getUpdateNoise()), instrument(false),
   sharedVars(s->get< bool >("coverage.sharedvars")),
   predecessors(s->get< bool >("coverage.predecessors")) {}
} MemoryAccessInstrumentationSettings;

// Definitions of analysis functions (callback functions called by PIN)
VOID initMemoryAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

// Definitions of helper functions
VOID setupAccessModule(Settings* settings);
VOID setupMemoryAccessSettings(MemoryAccessInstrumentationSettings& mais);

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
