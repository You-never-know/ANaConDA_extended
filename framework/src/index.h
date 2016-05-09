/**
 * @brief Contains definitions of functions for accessing various indexes.
 *
 * A file containing definitions of functions for accessing various indexes.
 *
 * @file      index.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-05-09
 * @version   0.5
 */

#ifndef __PINTOOL_ANACONDA__INDEX_H__
  #define __PINTOOL_ANACONDA__INDEX_H__

#include "pin.H"

#include "types.h"

// Definitions of functions for indexing various (framework) data
index_t indexImage(const IMAGE* image);
index_t indexFunction(const FUNCTION* function);
index_t indexCall(const CALL* call);
index_t indexInstruction(const INSTRUCTION* instruction);
index_t indexLocation(const LOCATION* location);

// Definitions of functions for indexing various (Intel PIN) data
index_t indexImage(const IMG img);
index_t indexFunction(const RTN rtn);
index_t indexCall(const INS ins);
index_t indexInstruction(const INS ins);
index_t indexLocation(const INS ins);

// Definitions of functions for accessing indexed data
const IMAGE* retrieveImage(index_t idx);
const FUNCTION* retrieveFunction(index_t idx);
const CALL* retrieveCall(index_t idx);
const INSTRUCTION* retrieveInstruction(index_t idx);
const LOCATION* retrieveLocation(index_t idx);

// Definitions of helper functions
VOID setupIndexModule();

#endif /* __PINTOOL_ANACONDA__INDEX_H__ */

/** End of file index.h **/
