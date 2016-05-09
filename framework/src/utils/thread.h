/**
 * @brief Contains definitions of thread-related utilities.
 *
 * A file containing definitions of various thread-related utility functions.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-04
 * @date      Last Update 2016-05-09
 * @version   0.3.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__THREAD_H__
  #define __PINTOOL_ANACONDA__UTILS__THREAD_H__

#include "pin.H"

#include "../types.h"

// Definitions of callback functions
typedef VOID (*THREADINITFUNPTR)(THREADID tid, VOID* data);

// Definitions of utility functions for registering thread init functions
VOID addThreadInitFunction(THREADINITFUNPTR callback, VOID* data);

// Definitions of utility functions for accessing thread information
THREADID getThreadId(THREAD thread);

// Definitions of utility functions for accessing backtrace information
index_t getLastBacktraceLocationIndex(THREADID tid);
std::string getLastBacktraceLocation(THREADID tid);
size_t getBacktraceSize(THREADID tid);

#endif /* __PINTOOL_ANACONDA__UTILS__THREAD_H__ */

/** End of file thread.h **/
