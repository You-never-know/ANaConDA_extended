/*
 * Copyright (C) 2019-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains information about the version of the framework.
 *
 * A file containing the implementation of functions that provide information
 *   about the version of the framework. This file also provides the default
 *   values of build-specific version information of the framework.
 *
 * @file      version.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-01-15
 * @date      Last Update 2019-01-27
 * @version   0.1
 */

#include "version.h"

#include "stdio.h"

// Default values of build-specific version information
#ifndef ANACONDA_BUILD
  #define ANACONDA_BUILD "<unknown>"
#endif

#ifndef ANACONDA_GIT_REVISION_SHORT
  #define ANACONDA_GIT_REVISION_SHORT NULL
#endif

#ifndef ANACONDA_GIT_REVISION_LONG
  #define ANACONDA_GIT_REVISION_LONG NULL
#endif

#ifndef ANACONDA_GIT_REVISION_DESCRIPTION_SHORT
  #define ANACONDA_GIT_REVISION_DESCRIPTION_SHORT "no git"
#endif

#ifndef ANACONDA_GIT_REVISION_DESCRIPTION_LONG
  #define ANACONDA_GIT_REVISION_DESCRIPTION_LONG "unknown git revision"
#endif

#ifndef ANACONDA_GIT_REVISION_IS_MODIFIED
  #define ANACONDA_GIT_REVISION_IS_MODIFIED 0
#endif

// Short version of the framework: <release> <build> (<rev-desc-short>)
const char* g_anacondaVersion = ANACONDA_RELEASE " " ANACONDA_BUILD
  " (" ANACONDA_GIT_REVISION_DESCRIPTION_SHORT ")";
// Long version of the framework: <release> build <build> (<rev-desc-long>)
const char* g_anacondaVersionLong = ANACONDA_RELEASE " build " ANACONDA_BUILD
  " (" ANACONDA_GIT_REVISION_DESCRIPTION_LONG ")";
// Release version of the framework
const char* g_anacondaReleaseVersion = ANACONDA_RELEASE;
// A number identifying a specific build of the framework
const char* g_anacondaBuildNumber = ANACONDA_BUILD;
// SHA1 hashes of the git revision from which the framework was built
const char* g_anacondaGitRevisionShort = ANACONDA_GIT_REVISION_SHORT;
const char* g_anacondaGitRevisionLong = ANACONDA_GIT_REVISION_LONG;
// A textual description of the git revision from which the framework was built
const char* g_anacondaGitRevisionDescriptionShort
  = ANACONDA_GIT_REVISION_DESCRIPTION_SHORT;
const char* g_anacondaGitRevisionDescriptionLong
  = ANACONDA_GIT_REVISION_DESCRIPTION_LONG;
// A flag indicating if the git revision was modified
bool g_anacondaGitRevisionIsModified = ANACONDA_GIT_REVISION_IS_MODIFIED;

/**
 * Gets a string containing the version of the framework.
 *
 * @return A string containing the version of the framework.
 */
const char* ANACONDA_GetVersion()
{
  return g_anacondaVersion;
}

/**
 * Gets a string containing a detailed version of the framework.
 *
 * @return A string containing a detailed version of the framework.
 */
const char* ANACONDA_GetVersionLong()
{
  return g_anacondaVersionLong;
}

/**
 * Gets a string containing the release version of the framework.
 *
 * @return A string containing the release version of the framework.
 */
const char* ANACONDA_GetReleaseVersion()
{
  return g_anacondaReleaseVersion;
}

/**
 * Gets a string containing a number identifying a specific build of the
 *   framework.
 *
 * @return A string containing a number identifying a specific build of the
 *   framework.
 */
const char* ANACONDA_GetBuildNumber()
{
  return g_anacondaBuildNumber;
}

/**
 * Gets a string containing a (short format) SHA1 hash of the git revision from
 *   which the framework was built.
 *
 * @return A string containing a (short format) SHA1 hash of the git revision
 *   from which the framework was built or @em NULL if the revision is unknown.
 */
const char* ANACONDA_GetGitRevision()
{
  return g_anacondaGitRevisionShort;
}

/**
 * Gets a string containing a (long format) SHA1 hash of the git revision from
 *   which the framework was built.
 *
 * @return A string containing a (long format) SHA1 hash of the git revision
 *   from which the framework was built or @em NULL if the revision is unknown.
 */
const char* ANACONDA_GetGitRevisionLong()
{
  return g_anacondaGitRevisionLong;
}

/**
 * Gets a description of the git revision from which the framework was built.
 *
 * @return A description of the git revision from which the framework was built.
 */
const char* ANACONDA_GetGitRevisionDescription()
{
  return g_anacondaGitRevisionDescriptionShort;
}

/**
 * Gets a detailed description of the git revision from which the framework was
 *   built.
 *
 * @return A detailed description of the git revision from which the framework
 *   was built.
 */
const char* ANACONDA_GetGitRevisionDescriptionLong()
{
  return g_anacondaGitRevisionDescriptionLong;
}

/**
 * Checks if the git revision from which the framework was built was modified.
 *
 * @return @em True if the revision from which the framework was built was
 *   modified, @em false if not or if there are no information about the
 *   revision.
 */
bool ANACONDA_GitRevisionIsModified()
{
  return g_anacondaGitRevisionIsModified;
}

/** End of file version.cpp **/
