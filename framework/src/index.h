/**
 * @brief Contains definitions of functions for accessing various indexes.
 *
 * A file containing definitions of functions for accessing various indexes.
 *
 * @file      index.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2016-03-22
 * @version   0.2
 */

#ifndef __PINTOOL_ANACONDA__INDEX_H__
  #define __PINTOOL_ANACONDA__INDEX_H__

#include "pin.H"

// Index types
typedef ADDRINT index_t;

// Special index values
#define INVALID_INDEX (index_t)-1;

// Functions for indexing various data
index_t indexImage(const std::string& name);
index_t indexFunction(const std::string& name);
index_t indexCall(const std::string& desc);
index_t indexInstruction(const std::string& dasm);

// Functions for accessing indexed data
std::string retrieveImage(index_t idx);
std::string retrieveFunction(index_t idx);
std::string retrieveCall(index_t idx);
std::string retrieveInstruction(index_t idx);

#endif /* __PINTOOL_ANACONDA__INDEX_H__ */

/** End of file index.h **/
