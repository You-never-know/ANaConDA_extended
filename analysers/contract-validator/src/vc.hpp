/*
 * Copyright (C) 2016-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of a vector clock.
 *
 * A file containing implementation of a vector clock and its operations.
 *
 * @file      vc.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-07-22
 * @version   0.2.0.1
 */

#ifndef __VC_HPP__
  #define __VC_HPP__

#include <algorithm>
#include <vector>

#include "pin.H"

// Type definitions
namespace vc {
  typedef UINT64 clock_t;
}

/**
 * @brief A structure representing a vector clock.
 *
 * The size of the vector clock is dynamic, positions that are not defined are
 * assumed to be zero as it means that there is no synchronisation between the
 * thread owning the vector clock and the thread on the missing position.
 */
typedef struct VectorClock_s
{
  /**
   * @brief A container used to store the vector clock.
   */
  typedef std::vector< vc::clock_t > Container;
  /**
   * @brief A position in the vector clock.
   */
  typedef Container::size_type Thread;

  Container vc; //!< Internal representation of the vector clock.

  /**
   * Initialises a vector clock of a specific thread.
   *
   * @param tid A thread whose vector clock should be initialised. The value of
   *   the vector clock at this position will be @c 1, all prior positions will
   *   be set to zero, positions after @em tid are assumed to be zero.
   */
  void init(Thread tid)
  {
    vc.assign(tid + 1, 0);
    vc[tid] = 1;
  }

  /**
   * Increments a vector clock of a specific thread.
   *
   * @param tid A thread whose vector clock should be incremented. The value of
   *   the vector clock at this position will be incremented by @c 1, all other
   *   positions will be left unchanged.
   */
  void increment(Thread tid)
  {
    ++vc[tid];
  }

  /**
   * Joins this vector clock with another vector clock. The join operation is
   *   defined as follows. Let @c VC1 and @c VC2 be two vector clocks,
   *   @c VC1.join(VC2) = for each position i: i = max(VC1(i), VC2(i))
   *
   * @param second A second vector clock to join with this vector clock.
   */
  void join(const VectorClock_s& second)
  {
    Thread min = std::min(this->vc.size(), second.vc.size());
    Thread max = std::max(this->vc.size(), second.vc.size());

    for (Thread i = 0; i < min; ++i)
    { // Compare clocks of threads specified in both vector clocks
      this->vc[i] = std::max(this->vc[i], second.vc[i]);
    }

    if (this->vc.size() == min)
    { // The second vector clock has clocks for more threads than this one, we
      // need to extend this vector clock with the clocks from the second one
      for (Thread i = min; i < max; ++i)
      { // The clocks from the second vector clock must be the maximum here
        this->vc.push_back(second.vc[i]);
      }
    }
  }

  /**
   * Checks the validity of a vector clock. A valid vector must be non-empty as
   *   during the initialisation at least its first position must have been set
   *   to a non-zero value.
   *
   * @return @em True if the vector clock is valid, @em false otherwise.
   */
  bool valid() const
  {
    return !vc.empty();
  }

  /**
   * Checks if an action performed by some thread happened before the action
   *   represented by this vector clock.
   *
   * @warning Calling @c this.hb(action,tid) checks if action happened-before
   *   this, not the opposite one may think!
   *
   * @param action A vector clock of the action performed by some thread.
   * @param tid A thread in which the @c action occurred.
   * @return @em True if the @c action happened-before the action represented
   *   by this vector clock, @em false otherwise.
   */
  bool hb(const VectorClock_s& action, Thread tid)
  {
    if (tid >= this->vc.size())
    { // We have not been synchronised with this thread yet, else we would have
      return false; // the information we are missing in our vector clock
    }

    // For an action to happens-before other action, its clock have to be lower
    return action.vc[tid] <= this->vc[tid]; // or equal the other threads clock
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
  if (!vc.valid()) return "[]";

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
