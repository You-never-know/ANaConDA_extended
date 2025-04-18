/*
 * Copyright (C) 2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of libdie.
 *
 * libdie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libdie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdie. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Contains information about the version of the library.
 *
 * A file containing the implementation of functions that provide information
 *   about the version of the library. This file also provides the default
 *   values of build-specific version information of the library.
 *
 * @file      version.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-01-03
 * @date      Last Update 2019-01-04
 * @version   0.2
 */

#include "version.h"

#include "stddef.h"

// Default values of build-specific version information
#ifndef LIBDIE_BUILD
  #define LIBDIE_BUILD "<unknown>"
#endif

#ifndef LIBDIE_GIT_REVISION_SHORT
  #define LIBDIE_GIT_REVISION_SHORT NULL
#endif

#ifndef LIBDIE_GIT_REVISION_LONG
  #define LIBDIE_GIT_REVISION_LONG NULL
#endif

#ifndef LIBDIE_GIT_REVISION_DESCRIPTION_SHORT
  #define LIBDIE_GIT_REVISION_DESCRIPTION_SHORT "no git"
#endif

#ifndef LIBDIE_GIT_REVISION_DESCRIPTION_LONG
  #define LIBDIE_GIT_REVISION_DESCRIPTION_LONG "unknown git revision"
#endif

#ifndef LIBDIE_GIT_REVISION_IS_MODIFIED
  #define LIBDIE_GIT_REVISION_IS_MODIFIED 0
#endif

// Short version of the library: <release> <build> (<rev-desc-short>)
const char* g_libdieVersion = LIBDIE_RELEASE " " LIBDIE_BUILD
  " (" LIBDIE_GIT_REVISION_DESCRIPTION_SHORT ")";
// Long version of the library: <release> build <build> (<rev-desc-long>)
const char* g_libdieVersionLong = LIBDIE_RELEASE " build " LIBDIE_BUILD
  " (" LIBDIE_GIT_REVISION_DESCRIPTION_LONG ")";
// Release version of the library
const char* g_libdieReleaseVersion = LIBDIE_RELEASE;
// A number identifying a specific build of the library
const char* g_libdieBuildNumber = LIBDIE_BUILD;
// SHA1 hashes of the git revision from which the library was built
const char* g_libdieGitRevisionShort = LIBDIE_GIT_REVISION_SHORT;
const char* g_libdieGitRevisionLong = LIBDIE_GIT_REVISION_LONG;
// A textual description of the git revision from which the library was built
const char* g_libdieGitRevisionDescriptionShort
  = LIBDIE_GIT_REVISION_DESCRIPTION_SHORT;
const char* g_libdieGitRevisionDescriptionLong
  = LIBDIE_GIT_REVISION_DESCRIPTION_LONG;
// A flag indicating if the git revision was modified
bool g_libdieGitRevisionIsModified = LIBDIE_GIT_REVISION_IS_MODIFIED;

/**
 * Gets a string containing the version of the library.
 *
 * @return A string containing the version of the library.
 */
const char* DIE_GetVersion()
{
  return g_libdieVersion;
}

/**
 * Gets a string containing a detailed version of the library.
 *
 * @return A string containing a detailed version of the library.
 */
const char* DIE_GetVersionLong()
{
  return g_libdieVersionLong;
}

/**
 * Gets a string containing the release version of the library.
 *
 * @return A string containing the release version of the library.
 */
const char* DIE_GetReleaseVersion()
{
  return g_libdieReleaseVersion;
}

/**
 * Gets a string containing a number identifying a specific build of the
 *   library.
 *
 * @return A string containing a number identifying a specific build of the
 *   library.
 */
const char* DIE_GetBuildNumber()
{
  return g_libdieBuildNumber;
}

/**
 * Gets a string containing a (short format) SHA1 hash of the git revision from
 *   which the library was built.
 *
 * @return A string containing a (short format) SHA1 hash of the git revision
 *   from which the library was built or @em NULL if the revision is unknown.
 */
const char* DIE_GetGitRevision()
{
  return g_libdieGitRevisionShort;
}

/**
 * Gets a string containing a (long format) SHA1 hash of the git revision from
 *   which the library was built.
 *
 * @return A string containing a (long format) SHA1 hash of the git revision
 *   from which the library was built or @em NULL if the revision is unknown.
 */
const char* DIE_GetGitRevisionLong()
{
  return g_libdieGitRevisionLong;
}

/**
 * Gets a description of the git revision from which the library was built.
 *
 * @return A description of the git revision from which the library was built.
 */
const char* DIE_GetGitRevisionDescription()
{
  return g_libdieGitRevisionDescriptionShort;
}

/**
 * Gets a detailed description of the git revision from which the library was
 *   built.
 *
 * @return A detailed description of the git revision from which the library was
 *   built.
 */
const char* DIE_GetGitRevisionDescriptionLong()
{
  return g_libdieGitRevisionDescriptionLong;
}

/**
 * Checks if the git revision from which the library was built was modified.
 *
 * @return @em True if the revision from which the library was built was
 *   modified, @em false if not or if there are no information about the
 *   revision.
 */
bool DIE_GitRevisionIsModified()
{
  return g_libdieGitRevisionIsModified;
}

/** End of file version.cpp **/
