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
