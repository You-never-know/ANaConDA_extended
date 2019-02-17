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
 * @brief Contains definitions of custom ELF functions.
 *
 * A file containing definitions of functions simplifying access to various
 *   information about ELF binaries (executable files, shared objects, etc.).
 *
 * @file      elfutils.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-09
 * @date      Last Update 2012-01-09
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__LINUX__ELFUTILS_H__
  #define __PINTOOL_ANACONDA__LINUX__ELFUTILS_H__

#include <map>
#include <string>

#include <gelf.h>

// Error aliases
#define ELF_ERROR 1
#define IO_ERROR 2

// Type definitions
typedef std::map< std::string, GElf_Shdr > GElf_Section_Map;

int gelf_getscns(const char* filename, GElf_Section_Map& sections);

#endif /* __PINTOOL_ANACONDA__LINUX__ELFUTILS_H__ */

/** End of file elfutils.h **/
