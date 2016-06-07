/**
 * @brief Contains definitions of functions for instrumenting unwind hooks.
 *
 * A file containing definitions of functions for instrumenting unwind hooks.
 *
 * @file      unwind.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-07
 * @date      Last Update 2016-06-07
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__
  #define __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__

#include "pin.H"

// Type definitions
typedef VOID (*UNWINDFUNPTR)(THREADID tid, ADDRINT sp);

// Functions for instrumenting unwind hooks
VOID instrumentUnwindFunction(RTN rtn, UNWINDFUNPTR callback);

#endif /* __ANACONDA_FRAMEWORK__UTILS__UNWIND_H__ */

/** End of file unwind.h **/
