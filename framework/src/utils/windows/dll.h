/*
 * Copyright (C) 2015-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of functions for managing DLLs.
 *
 * A file containing definitions of functions for managing DLLs.
 *
 * @file      dll.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-07-17
 * @date      Last Update 2015-07-17
 * @version   0.2
 */

#ifndef __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__
  #define __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__

#include <windows.h>

HMODULE getModuleHandleByAddress(void* address);
HMODULE getHiddenAnacondaFrameworkHandle();
HMODULE getHiddenPinFrameworkHandle();

#endif /* __ANACONDA_FRAMEWORK__UTILS__WINDOWS__DLL_H__ */

/** End of file dll.h **/
