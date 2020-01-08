/*
 * Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

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
