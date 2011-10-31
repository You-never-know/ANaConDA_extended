/**
 * @brief A file containing definitions of access-related callback functions.
 *
 * A file containing definitions of callback functions called when some data
 *   are read from a memory or written to a memory.
 *
 * @file      access.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-10-19
 * @date      Last Update 2011-10-31
 * @version   0.1.2
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__

#include "pin.H"

VOID beforeMemoryRead(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT readAddr,
  INT32 size, CONTEXT *registers);
VOID beforeMemoryRead2(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT readAddr1,
  ADDRINT readAddr2, INT32 size, CONTEXT *registers);

VOID beforeMemoryWrite(ADDRINT rtnAddr, ADDRINT insAddr, ADDRINT writtenAddr,
  INT32 size, ADDRINT memAddr, INT32 memSize, CONTEXT *registers);
VOID beforeMemoryWriteValue(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, ADDRINT value, CONTEXT *registers);
VOID beforeMemoryWriteXmmReg(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);
VOID beforeMemoryWriteYmmReg(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);
VOID beforeMemoryWriteX87Reg(ADDRINT rtnAddr, ADDRINT insAddr,
  ADDRINT writtenAddr, INT32 size, PIN_REGISTER *value, CONTEXT *registers);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__ACCESS_H__ */

/** End of file access.h **/
