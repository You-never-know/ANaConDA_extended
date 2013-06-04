/**
 * @brief Contains definitions of thread-related utilities.
 *
 * A file containing definitions of various thread-related utility functions.
 *
 * @file      thread.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-04
 * @date      Last Update 2013-06-04
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__THREAD_H__
  #define __PINTOOL_ANACONDA__UTILS__THREAD_H__

#include "pin.H"

// Definitions of callback functions
typedef VOID (*THREADINITFUNPTR)(THREADID tid, VOID* data);

// Definitions of utility functions
VOID addThreadInitFunction(THREADINITFUNPTR callback, VOID* data);

#endif /* __PINTOOL_ANACONDA__UTILS__THREAD_H__ */

/** End of file thread.h **/
