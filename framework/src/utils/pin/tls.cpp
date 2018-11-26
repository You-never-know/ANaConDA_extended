/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of TLS-related helper functions.
 *
 * A file containing implementation of functions for working with PIN's thread
 *   local storage (TLS).
 *
 * @file      tls.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-04
 * @date      Last Update 2012-02-04
 * @version   0.1
 */

#include "tls.h"

/**
 * Allocates a new TLS key and associate it with a given destruction function.
 *
 * @param dfunc A function called before a thread finishes.
 * @return A new TLS key or -1 if the key cannot be created.
 */
TLS_KEY TLS_CreateThreadDataKey(DESTRUCTFUN dfunc)
{
  return PIN_CreateThreadDataKey(dfunc);
}

/**
 * Gets data stored in a specific TLS slot of a thread.
 *
 * @param key A TLS key identifying the slot where the data are stored.
 * @param tid A number uniquely identifying the thread.
 * @return The data stored in the slot.
 */
VOID* TLS_GetThreadData(TLS_KEY key, THREADID tid)
{
  return PIN_GetThreadData(key, tid);
}

/**
 * Stores data in a specific TLS slot of a thread.
 *
 * @param key A TLS key identifying the slot where the data should be stored.
 * @param data The data which should be stored in the slot.
 * @param tid A number uniquely identifying the thread.
 * @return @em True if the specified key is allocated, @em false otherwise.
 */
BOOL TLS_SetThreadData(TLS_KEY key, const VOID* data, THREADID tid)
{
  return PIN_SetThreadData(key, data, tid);
}

/** End of file tls.cpp **/
