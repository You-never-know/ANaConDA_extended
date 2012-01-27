/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2012-01-27
 * @version   0.1.4
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
VOID beforeMemoryRead(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT readAddr, INT32 size, CONTEXT *registers);
VOID beforeMemoryRead2(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT readAddr1, ADDRINT readAddr2, INT32 size, CONTEXT *registers);

VOID beforeMemoryWrite(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, ADDRINT memAddr, INT32 memSize,
  CONTEXT *registers);
VOID beforeMemoryWriteValue(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, ADDRINT value, CONTEXT *registers);
VOID beforeMemoryWriteXmmReg(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);
VOID beforeMemoryWriteYmmReg(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);
VOID beforeMemoryWriteX87Reg(THREADID tid, ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);

// Definitions of callback functions
typedef VOID (*TYPE1READFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);
typedef VOID (*TYPE1WRITEFUNPTR)(THREADID tid, ADDRINT addr, UINT32 size,
  const VARIABLE& variable);

// Definitions of functions for registering callback functions
VOID ACCESS_BeforeMemoryRead(TYPE1READFUNPTR callback);
VOID ACCESS_BeforeMemoryWrite(TYPE1WRITEFUNPTR callback);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
