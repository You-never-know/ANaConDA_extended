/*
 * Copyright (C) 2013-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of useful compile-time operations.
 *
 * A file containing implementation of functions which can be fully evaluated
 *   during the compile-time.
 *
 * @file      ctops.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-09-19
 * @date      Last Update 2015-06-01
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__
  #define __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__

#include "../defs.h"

namespace ctops
{

/**
 * Tells the compiler that it reached the end of the list.
 *
 * @tparam T A type of item searched for.
 * @tparam ITEM The item searched for.
 *
 * @return Always @em false, as the item cannot be in an empty list.
 */
template< typename T, T ITEM >
CONSTEXPR bool contains()
{
  return false;
}

/**
 * Checks if a list contains an item at compile-time.
 *
 * @tparam T A type of items in the list.
 * @tparam ITEM The item searched for.
 * @tparam HEAD The first item in the list.
 * @tparam TAIL The remaining items in the list.
 *
 * @return @em True if the list contains the item, @em false otherwise.
 */
template< typename T, T ITEM, T HEAD, T... TAIL >
CONSTEXPR bool contains()
{
  return ITEM == HEAD || contains< T, ITEM, TAIL... >();
}

}

#endif /* __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__ */

/** End of file ctops.hpp **/
