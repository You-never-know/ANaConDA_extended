/*
 * Copyright (C) 2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * A file containing the definition of functions that provide information about
 *   the version of the framework. This file also defines the release version
 *   of the framework.
 *
 * @file      version.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2019-01-15
 * @date      Last Update 2019-01-27
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__VERSION_H__
  #define __ANACONDA_FRAMEWORK__VERSION_H__

// Current release version of the framework
#define ANACONDA_RELEASE "0.3"

// Functions for accessing version information
const char* ANACONDA_GetVersion();
const char* ANACONDA_GetVersionLong();
const char* ANACONDA_GetReleaseVersion();
const char* ANACONDA_GetBuildNumber();
const char* ANACONDA_GetGitRevision();
const char* ANACONDA_GetGitRevisionLong();
const char* ANACONDA_GetGitRevisionDescription();
const char* ANACONDA_GetGitRevisionDescriptionLong();
bool ANACONDA_GitRevisionIsModified();

#endif /* __ANACONDA_FRAMEWORK__VERSION_H__ */

/** End of file version.h **/
