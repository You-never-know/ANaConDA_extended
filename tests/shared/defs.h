/**
 * @brief Contains definitions shared among tests.
 *
 * A file containing definitions shared among tests.
 *
 * @file      defs.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-08
 * @date      Last Update 2016-07-26
 * @version   0.1.1
 */

#ifndef __ANACONDA_TESTS__SHARED__DEFS_H__
  #define __ANACONDA_TESTS__SHARED__DEFS_H__

#include <stdio.h>

#ifndef __func__
  #define __func__ __FUNCTION__
#endif

// Macros for flagging the beginning and the end of a function
#define FUNCTION_START \
  printf("%s: started\n", __func__); \
  fflush(stdout);
#define FUNCTION_EXIT \
  printf("%s: exited\n", __func__); \
  fflush(stdout);

#endif /* __ANACONDA_TESTS__SHARED__DEFS_H__ */

/** End of file defs.h **/
