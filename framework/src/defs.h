/**
 * @brief Contains definitions shared among various parts of the framework.
 *
 * A file containing definitions shared among various parts of the framework.
 *
 * @file      defs.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-05-28
 * @date      Last Update 2012-06-01
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__DEFS_H__
  #define __PINTOOL_ANACONDA__DEFS_H__

#ifdef TARGET_WINDOWS
  #define API_FUNCTION __declspec(dllexport)
#else
  #define API_FUNCTION
#endif

#endif /* __PINTOOL_ANACONDA__DEFS_H__ */

/** End of file defs.h **/
