/**
 * @brief Contains implementation of custom dynamic loader functions.
 *
 * A file containing implementation of functions simplifying access to various
 *   information about dynamic loader (loaded shared objects, their addresses
 *   etc.).
 *
 * @file      dlutils.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-20
 * @date      Last Update 2012-01-21
 * @version   0.1
 */

#include "dlutils.h"

#include <string.h>

/**
 * Searches for a specific shared object.
 *
 * @param info A structure containing information about a shared object.
 * @param size A size of the structure pointed to by @em info.
 * @param data A structure where the information about the searched shared
 *   object should be stored.
 * @return 1 if the searched shared object was found, 0 otherwise.
 */
static
int dl_get_sobj_callback(struct dl_phdr_info* info, size_t size, void* data)
{
  if (strcmp(info->dlpi_name, ((dl_sobj_info*)data)->dlsi_name) == 0)
  { // Found the shared object, store its base address to the output structure
    ((dl_sobj_info*)data)->dlsi_addr = info->dlpi_addr;
    // Signal that the searched object was found, it also stops the search
    return 1;
  }

  // Not searching for this object, continue the search
  return 0;
}

/**
 * Stores information about all found shared objects to a list.
 *
 * @param info A structure containing information about a shared object.
 * @param size A size of the structure pointed to by @em info.
 * @param data A pointer to a list where the information about the shared
 *   object should be stored.
 * @return 0 if the information about the shared object was successfully stored
 *   in the list.
 */
static
int dl_get_sobjs_callback(struct dl_phdr_info* info, size_t size, void* data)
{
  // Store the information about the shared object to the list
  ((dl_sobj_info_list*)data)->push_back(dl_sobj_info(info->dlpi_name,
    info->dlpi_addr));

  // No error occurred (list should always be valid and info non-NULL)
  return 0;
}

/**
 * Gets information about a shared object.
 *
 * @param name A name of the shared object.
 * @return A structure containing information about the shared object.
 */
dl_sobj_info dl_get_sobj(const char* name)
{
  // The name of the shared object is already known, address not (set 0)
  dl_sobj_info info(name, 0);

  // Try to find the remaining information about the shared object
  dl_iterate_phdr(dl_get_sobj_callback, &info);

  // Return all information which were found
  return info;
}

/**
 * Gets information about all shared objects loaded by the application which
 *   called this function.
 *
 * @param names A list to which should be the information stored.
 * @return 0 if the information were retrieved successfully.
 */
int dl_get_sobjs(dl_sobj_info_list& sobjs)
{
  // Get information about all shared objects loaded by the application
  dl_iterate_phdr(dl_get_sobjs_callback, &sobjs);

  // No errors occurred
  return 0;
}

/** End of file dlutils.cpp **/
