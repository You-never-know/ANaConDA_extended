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
