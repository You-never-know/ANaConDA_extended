/**
 * @brief Contains definitions of types used in various parts of the framework.
 *
 * A file containing definitions of types used in various parts of the
 *   framework.
 *
 * @file      types.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-13
 * @date      Last Update 2013-02-13
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__TYPES_H__
  #define __PINTOOL_ANACONDA__TYPES_H__

#include "pin.H"

// Definitions of classes representing synchronisation primitives
typedef class INDEX< 200 > LOCK; //!< A class representing a lock.
typedef class INDEX< 201 > COND; //!< A class representing a condition.

#endif /* __PINTOOL_ANACONDA__TYPES_H__ */

/** End of file types.h **/
