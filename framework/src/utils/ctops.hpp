/**
 * @brief Contains implementation of useful compile-time operations.
 *
 * A file containing implementation of functions which can be fully evaluated
 *   during the compile-time.
 *
 * @file      ctops.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-09-19
 * @date      Last Update 2013-09-20
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__
  #define __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__

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
constexpr bool contains()
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
constexpr bool contains()
{
  return ITEM == HEAD || contains< T, ITEM, TAIL... >();
}

}

#endif /* __PINTOOL_ANACONDA__UTILS__CTOPS_HPP__ */

/** End of file ctops.hpp **/
