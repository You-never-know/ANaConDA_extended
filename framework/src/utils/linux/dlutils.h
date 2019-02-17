/*
 * Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of custom dynamic loader functions.
 *
 * A file containing definitions of functions simplifying access to various
 *   information about dynamic loader (loaded shared objects, their addresses
 *   etc.).
 *
 * @file      dlutils.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-20
 * @date      Last Update 2012-01-21
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__LINUX__DLUTILS_H__
  #define __PINTOOL_ANACONDA__LINUX__DLUTILS_H__

#include <link.h>

#include <list>

/**
 * @brief A structure containing information about a shared object.
 */
struct dl_sobj_info
{
  const char* dlsi_name; //!< A name of a shared object.
  ElfW(Addr) dlsi_addr; //!< A base address of a shared object.

  /**
   * Constructs a dl_sobj_info object.
   *
   * @param name A name of a shared object.
   * @param addr A base address of a shared object.
   */
  dl_sobj_info(const char* name, ElfW(Addr) addr) : dlsi_name(name),
    dlsi_addr(addr) {}
};

// Type definitions
typedef std::list< dl_sobj_info > dl_sobj_info_list;

dl_sobj_info dl_get_sobj(const char* name);
int dl_get_sobjs(dl_sobj_info_list& sobjs);

#endif /* __PINTOOL_ANACONDA__LINUX__DLUTILS_H__ */

/** End of file dlutils.h **/
