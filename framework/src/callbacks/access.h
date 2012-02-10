/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-02-10
 * @version   0.3
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

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

// Definitions of analysis functions (callback functions called by PIN)
VOID initAccessTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);

VOID beforeMemoryRead(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers);
VOID beforeMemoryWrite(THREADID tid, ADDRINT addr, UINT32 size, UINT32 memOpIdx,
  ADDRINT rtnAddr, ADDRINT insAddr, CONTEXT* registers);

VOID afterMemoryRead(THREADID tid, UINT32 memOpIdx);
VOID afterMemoryWrite(THREADID tid, UINT32 memOpIdx);

// Definitions of callback functions
typedef VOID (*TYPE1READFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*TYPE1WRITEFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);

// Definitions of functions for registering callback functions
VOID ACCESS_BeforeMemoryRead(TYPE1READFUNPTR callback);
VOID ACCESS_BeforeMemoryWrite(TYPE1WRITEFUNPTR callback);

VOID ACCESS_AfterMemoryRead(TYPE1READFUNPTR callback);
VOID ACCESS_AfterMemoryWrite(TYPE1WRITEFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
