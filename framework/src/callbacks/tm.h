/**
 * @brief Contains definitions of functions for monitoring transactional memory.
 *
 * A file containing definitions of functions for monitoring transactional
 *   memory (TM) operations.
 *
 * @file      tm.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-06
 * @date      Last Update 2013-06-17
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__TM_H__
  #define __PINTOOL_ANACONDA__CALLBACKS__TM_H__

#include "pin.H"

#include "../settings.h"

// Definitions of functions for configuring TM monitoring
VOID setupTmModule(Settings* settings);

#endif /* __PINTOOL_ANACONDA__CALLBACKS__TM_H__ */

/** End of file tm.h **/
