/*
 * Copyright (C) 2013-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of functions shared among callback modules.
 *
 * A file containing implementation of functions shared among callback modules.
 *
 * @file      shared.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-06-12
 * @date      Last Update 2013-06-12
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__CALLBACKS__SHARED_HPP__
  #define __PINTOOL_ANACONDA__CALLBACKS__SHARED_HPP__

#include "pin.H"

#include "../settings.h"

/**
 * Maps an argument of a function to an object representing this argument, e.g.,
 *   thread, lock, condition, etc.
 *
 * @param arg A pointer to the argument, i.e., data passed to the function.
 * @param hi A structure containing information about the function.
 * @return An object representing the argument.
 */
template< typename T >
inline
T mapArgTo(ADDRINT* arg, HookInfo* hi)
{
  for (int depth = hi->refdepth; depth > 0; --depth)
  { // The pointer points to another pointer, not to the data, dereference it
    arg = reinterpret_cast< ADDRINT* >(*arg);
  }

  T obj; // Map the argument to a concrete object using the mapper object
  obj.q_set(hi->mapper->map(arg));

  // The created object must be valid (the mapper object cannot return 0)
  assert(obj.is_valid());

  return obj; // Return the object representing the argument
}

#endif /* __PINTOOL_ANACONDA__CALLBACKS__SHARED_HPP__ */

/** End of file shared.hpp **/
