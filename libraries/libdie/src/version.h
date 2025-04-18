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
 * A file containing the definition of functions that provide information about
 *   the version of the library. This file also defines the release version of
 *   the library.
 *
 * @file      version.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-01-03
 * @date      Last Update 2019-01-30
 * @version   0.2.2
 */

#ifndef __LIBDIE__VERSION_H__
  #define __LIBDIE__VERSION_H__

// Current release version of the library
#define LIBDIE_RELEASE "0.3"

// Functions for accessing version information
const char* DIE_GetVersion();
const char* DIE_GetVersionLong();
const char* DIE_GetReleaseVersion();
const char* DIE_GetBuildNumber();
const char* DIE_GetGitRevision();
const char* DIE_GetGitRevisionLong();
const char* DIE_GetGitRevisionDescription();
const char* DIE_GetGitRevisionDescriptionLong();
bool DIE_GitRevisionIsModified();

#endif /* __LIBDIE__VERSION_H__ */

/** End of file version.h **/
