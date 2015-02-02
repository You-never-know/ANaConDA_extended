/**
 * @brief Contains vector clock implementation.
 *
 * A file containing implementation of a vector clock and its operations.
 *
 * @file      vc.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-01-30
 * @date      Last Update 2015-02-02
 * @version   0.2
 */

#ifndef __VC_HPP__
  #define __VC_HPP__

#include "pin.H"

#include <vector>

// Type definitions
namespace vc {
  typedef unsigned long clock_t;
}

/**
 * @brief A structure representing a vector clock.
 */
typedef struct VectorClock_s
{
  /**
   * @brief A type of the container used to store a vector clock.
   */
  typedef std::vector< vc::clock_t > VectorClockContainer;

  VectorClockContainer vc; //!< Internal representation of a vector clock.

  /**
   * Initialises a vector clock.
   *
   * @param tid A thread whose vector clock should be initialised.
   */
  void init(VectorClockContainer::size_type tid)
  {
    vc.assign(tid + 1, 0);
    vc[tid] = 1;
  }

  /**
   * Increments a vector clock.
   *
   * @param tid A thread whose vector clock should be incremented.
   */
  void increment(VectorClockContainer::size_type tid)
  {
    ++vc[tid];
  }
} VectorClock;

/**
 * Concatenates a string with a vector clock.
 *
 * @param s A string.
 * @param vc A vector clock.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em vc.
 */
inline
std::string operator+(const std::string& s, const VectorClock& vc)
{
  std::string result = s + "[";

  for (unsigned int i = 0; i < vc.vc.size(); ++i)
  {
    result += decstr(vc.vc[i]) + ",";
  }

  result[result.length() - 1] = ']';

  return result;
}

#endif /* __VC_HPP__ */

/** End of file vc.hpp **/
