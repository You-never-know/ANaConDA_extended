/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions influencing the compilation of the framework.
 *
 * A file containing definitions influencing the compilation of the framework.
 *
 * @file      config.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-11-01
 * @date      Last Update 2019-06-04
 * @version   0.4
 */

#ifndef __PINTOOL_ANACONDA__CONFIG_H__
  #define __PINTOOL_ANACONDA__CONFIG_H__

/**
 * @brief Compiles the framework so that it prints information about a function
 *   when the function is about to be executed (information printed before the
 *   function is actually executed, i.e., before the first instruction of the
 *   function is executed).
 */
#define ANACONDA_PRINT_EXECUTED_FUNCTIONS 0

/**
 * @brief Compiles the framework so that it prints information about backtrace
 *   construction. Information about functions added to and removed from the
 *   call stack will be printed.
 */
#define ANACONDA_PRINT_BACKTRACE_CONSTRUCTION 0

/**
 * @brief Compiles the framework so that it prints information about injected
 *   noise.
 */
#define ANACONDA_PRINT_INJECTED_NOISE 0

/**
 * @brief Compiles the framework so that it prints detailed information about
 *   the calls encountered in the execution of a program.
 */
#define ANACONDA_DEBUG_CALL_TRACKING 0

/**
 * @brief Compiles the framework so that it prints detailed information about
 *   the functions encountered in the execution of a program.
 */
#define ANACONDA_DEBUG_FUNCTION_TRACKING 0

/**
 * @brief Compiles the framework so that it prints detailed information about
 *   missed callback triggers for memory accesses.
 */
#define ANACONDA_DEBUG_MEMORY_ACCESSES 0

#endif /* __PINTOOL_ANACONDA__CONFIG_H__ */

/** End of file config.h **/
