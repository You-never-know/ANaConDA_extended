/*
 * Copyright (C) 2012-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of functions for accessing environment variables.
 *
 * A file containing definitions of functions for accessing environment
 *   variables.
 *
 * @file      env.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-18
 * @date      Last Update 2013-05-30
 * @version   0.1.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__ENV_H__
  #define __PINTOOL_ANACONDA__UTILS__ENV_H__

#include <map>
#include <string>

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;

void getEnvVars(EnvVarMap& envVars);

#endif /* __PINTOOL_ANACONDA__UTILS__ENV_H__ */

/** End of file env.h **/
