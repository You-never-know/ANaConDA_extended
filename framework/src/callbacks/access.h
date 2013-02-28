/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2013-02-28
 * @version   0.6.2.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

#include "../defs.h"
#include "../settings.h"
#include "../types.h"

/**
 * @brief An enumeration describing the types of various callback functions.
 */
typedef enum CallbackType_e
{
  CLBK_NONE  = 0x0, //!< No callback function registered.
  CLBK_TYPE0 = 0x1, //!< Type 0 callback function registered.
  CLBK_TYPE1 = 0x2, //!< Type 1 callback function registered.
  CLBK_TYPE2 = 0x4  //!< Type 2 callback function registered.
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
  NoiseDesc* noise;

  /**
   * Constructs an InstrumentationSettings_s object.
   */
  InstrumentationSettings_s() : beforeCallback(NULL), beforeRepCallback(NULL),
    afterCallback(NULL), afterRepCallback(NULL), beforeCallbackType(CLBK_NONE),
    afterCallbackType(CLBK_NONE), noise(NULL) {}

  /**
   * Constructs an InstrumentationSettings_s object.
   *
   * @param n A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  InstrumentationSettings_s(NoiseDesc* n) : beforeCallback(NULL),
    beforeRepCallback(NULL), afterCallback(NULL), afterRepCallback(NULL),
    beforeCallbackType(CLBK_NONE), afterCallbackType(CLBK_NONE), noise(n) {}
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
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   */
  MemoryAccessInstrumentationSettings_s() : reads(), writes(), updates(),
    instrument(false), sharedVars(false) {}

  /**
   * Constructs a MemoryAccessSettings_s object.
   *
   * @param s An object containing the ANaConDA framework's settings.
   */
  MemoryAccessInstrumentationSettings_s(Settings* s) : reads(s->getReadNoise()),
   writes(s->getWriteNoise()), updates(s->getUpdateNoise()), instrument(false),
   sharedVars(s->get< bool >("coverage.sharedvars")) {}
} MemoryAccessInstrumentationSettings;

// Definitions of analysis functions (callback functions called by PIN)
VOID initMemoryAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

// Definitions of helper functions
VOID setupAccessModule(Settings* settings);
VOID setupMemoryAccessSettings(MemoryAccessInstrumentationSettings& mais);

// Definitions of callback functions
typedef VOID (*MEMREAD1FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMREAD2FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMWRITE1FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMWRITE2FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);
typedef VOID (*MEMUPDATE1FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*MEMUPDATE2FUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable, const LOCATION& location);

// Definitions of functions for registering callback functions
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREAD1FUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryRead(MEMREAD2FUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITE1FUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeMemoryWrite(MEMWRITE2FUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATE1FUNPTR callback);
API_FUNCTION VOID ACCESS_BeforeAtomicUpdate(MEMUPDATE2FUNPTR callback);

API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREAD1FUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryRead(MEMREAD2FUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITE1FUNPTR callback);
API_FUNCTION VOID ACCESS_AfterMemoryWrite(MEMWRITE2FUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATE1FUNPTR callback);
API_FUNCTION VOID ACCESS_AfterAtomicUpdate(MEMUPDATE2FUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
