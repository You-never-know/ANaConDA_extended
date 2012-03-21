/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-03-21
 * @version   0.5
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

#include "../settings.h"

/**
 * @brief A structure representing a variable.
 */
typedef struct Variable_s
{
  std::string name; //!< A name of the variable.
  std::string type; //!< A type of the variable.
  UINT32 offset; //!< An offset within the variable which was accessed.

  /**
   * Constructs a Variable_s object.
   */
  Variable_s() : name(), type(), offset(0) {}

  /**
   * Constructs a Variable_s object.
   *
   * @param n A name of a variable.
   * @param t A type of a variable.
   * @param o An offset within a variable which was accessed.
   */
  Variable_s(const std::string& n, const std::string& t, const UINT32 o)
    : name(n), type(t), offset(o) {}
} VARIABLE;

/**
 * @brief A structure representing a source code location.
 */
typedef struct Location_s
{
  std::string file; //!< A name of a file.
  INT32 line; //!< A line number.

  /**
   * Constructs a Location_s object.
   */
  Location_s() : file(), line(-1) {}
} LOCATION;

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
  AFUNPTR afterCallback; //!< A function called after an instrumented object.
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
  InstrumentationSettings_s() : beforeCallback(NULL), afterCallback(NULL),
    beforeCallbackType(CLBK_NONE), afterCallbackType(CLBK_NONE), noise(NULL) {}

  /**
   * Constructs an InstrumentationSettings_s object.
   *
   * @param n A structure containing detailed information about a noise which
   *   should be inserted before an instrumented object.
   */
  InstrumentationSettings_s(NoiseDesc* n) : beforeCallback(NULL),
    afterCallback(NULL), beforeCallbackType(CLBK_NONE),
    afterCallbackType(CLBK_NONE), noise(n) {}
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
   * @brief A flag determining if instrumenting memory accesses is necessary,
   *   i.e., set to @em true if at least one callback function is registered
   *   and to @em false if no callback function is registered.
   */
  bool instrument;

  /**
   * Constructs a MemoryAccessInstrumentationSettings_s object.
   */
  MemoryAccessInstrumentationSettings_s() : reads(), writes() {}

  /**
   * Constructs a MemoryAccessSettings_s object.
   *
   * @param s An object containing the ANaConDA framework's settings.
   */
  MemoryAccessInstrumentationSettings_s(Settings* s) : reads(s->getReadNoise()),
   writes(s->getWriteNoise()) {}
} MemoryAccessInstrumentationSettings;

// Definitions of analysis functions (callback functions called by PIN)
VOID initMemoryAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

// Definitions of helper functions
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

// Definitions of functions for registering callback functions
VOID ACCESS_BeforeMemoryRead(MEMREAD1FUNPTR callback);
VOID ACCESS_BeforeMemoryRead(MEMREAD2FUNPTR callback);
VOID ACCESS_BeforeMemoryWrite(MEMWRITE1FUNPTR callback);
VOID ACCESS_BeforeMemoryWrite(MEMWRITE2FUNPTR callback);

VOID ACCESS_AfterMemoryRead(MEMREAD1FUNPTR callback);
VOID ACCESS_AfterMemoryRead(MEMREAD2FUNPTR callback);
VOID ACCESS_AfterMemoryWrite(MEMWRITE1FUNPTR callback);
VOID ACCESS_AfterMemoryWrite(MEMWRITE2FUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
