/**
 * @brief Contains implementation of custom ELF functions.
 *
 * A file containing implementation of functions simplifying access to various
 *   information about ELF binaries (executable files, shared objects, etc.).
 *
 * @file      elfutils.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-01-09
 * @date      Last Update 2012-01-09
 * @version   0.1
 */

#include "elfutils.h"

#include <fcntl.h>

#include <sys/stat.h>

/**
 * Gets information about all sections in an ELF binary file.
 *
 * @param filename A name of the ELF binary file.
 * @param sections A map to which should be the information about the sections
 *   stored.
 * @return 0 if the information about the sections were extracted successfully,
 *   ELF_ERROR if some error in the libelf library occurred or IO_ERROR if the
 *   ELF binary file could not be opened.
 */
int gelf_getscns(const char* filename, GElf_Section_Map& sections)
{
  // Helper variables
  int fd;
  char* name;
  size_t shstrndx;
  Elf* elf = NULL;
  Elf_Scn* scn = NULL;
  GElf_Shdr shdr;

  if (elf_version(EV_CURRENT) == EV_NONE)
  { // Could not set the ELF version, libELF library cannot be initialised
    return ELF_ERROR;
  }

  if ((fd = open(filename, O_RDONLY , 0)) < 0)
  { // Could not open the ELF binary for reading
    return IO_ERROR;
  }

  if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
  { // Could not open the ELF binary for further processing
    return ELF_ERROR;
  }

  if (elf_getshdrstrndx(elf, &shstrndx) != 0)
  { // Could not obtain the section index of the string table
    return ELF_ERROR;
  }

  while ((scn = elf_nextscn(elf, scn)) != NULL)
  { // Extract information from all sections in the ELF binary
    if (gelf_getshdr(scn, &shdr) == &shdr)
    { // Store only sections with successfully extracted information about them
      if ((name = elf_strptr(elf, shstrndx, shdr.sh_name)) != NULL)
      { // Store only sections whose names can be retrieved (ignore all other)
        sections[name] = shdr;
      }
    }
  }

  // Close the ELF binary
  elf_end(elf);
  close(fd);

  // No errors occurred (information extraction functions should never fail)
  return 0;
}

/** End of file elfutils.cpp **/
