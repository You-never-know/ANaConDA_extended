/**
 * @brief Contains definitions of functions for accessing various indexes.
 *
 * A file containing definitions of functions for accessing various indexes.
 *
 * @file      index.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-07-27
 * @date      Last Update 2012-09-07
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__INDEX_H__
  #define __PINTOOL_ANACONDA__INDEX_H__

#include "pin.H"

typedef ADDRINT index_t;

index_t indexImage(const std::string& name);
index_t indexFunction(const std::string& name);
index_t indexCall(const std::string& desc);

const std::string& retrieveImage(index_t idx);
const std::string& retrieveFunction(index_t idx);
const std::string& retrieveCall(index_t idx);

#endif /* __PINTOOL_ANACONDA__INDEX_H__ */

/** End of file index.h **/
