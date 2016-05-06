/**
 * @brief Contains definitions of functions for accessing various indexes.
 *
 * A file containing definitions of functions for accessing various indexes.
 *
 * @file      index.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-05-06
 * @version   0.3
 */

#ifndef __PINTOOL_ANACONDA__INDEX_H__
  #define __PINTOOL_ANACONDA__INDEX_H__

#include "types.h"

// Functions for indexing various data
index_t indexImage(const IMAGE* image);
index_t indexFunction(const FUNCTION* function);
index_t indexCall(const CALL* call);
index_t indexInstruction(const INSTRUCTION* instruction);

// Functions for accessing indexed data
const IMAGE* retrieveImage(index_t idx);
const FUNCTION* retrieveFunction(index_t idx);
const CALL* retrieveCall(index_t idx);
const INSTRUCTION* retrieveInstruction(index_t idx);

#endif /* __PINTOOL_ANACONDA__INDEX_H__ */

/** End of file index.h **/
