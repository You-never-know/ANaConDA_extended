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
 * @brief Contains vector clock implementation.
 *
 * A file containing implementation of a vector clock and its operations.
 *
 * @file      vc.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-01-30
 * @date      Last Update 2015-06-08
 * @version   0.6.1
 */

#ifndef __VC_HPP__
  #define __VC_HPP__

#include "pin.H"

#include <algorithm>
#include <vector>

// Type definitions
namespace vc {
  typedef UINT64 clock_t;
}

typedef std::set< THREADID > Threads;

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

  /**
   * Updates a vector clock.
   *
   * @param tid A thread whose vector clock should be updated.
   * @param clk The current clock for thread @e tid.
   */
  void update(VectorClockContainer::size_type tid, vc::clock_t clk)
  {
    for (VectorClockContainer::size_type i = vc.size(); i <= tid; ++i)
      vc.push_back(0);

    vc[tid] = clk;
  }

  /**
   * Joins this vector clock with another vector clock.
   *
   * @param second A second vector clock to join with this vector clock.
   */
  void join(const VectorClock_s& second)
  {
    VectorClockContainer::size_type min = std::min(this->vc.size(), second.vc.size());
    VectorClockContainer::size_type max = std::max(this->vc.size(), second.vc.size());

    for (VectorClockContainer::size_type i = 0; i < min; ++i)
    { // Compare clocks of threads specified in both vector clocks
      this->vc[i] = std::max(this->vc[i], second.vc[i]);
    }

    if (this->vc.size() == min)
    { // This vector clock has not clocks for some threads specified
      for (VectorClockContainer::size_type i = min; i < max; ++i)
      { // The clocks in the second vector clock will be maximum here
        this->vc.push_back(second.vc[i]);
      }
    }
  }

  /**
   * Computes a set of threads in which the operation represented by this vector
   *   clock was already executed.
   *
   * @param threads A set of threads in which the operation represented by this
   *   vector clock was already executed.
   */
  void seen(Threads& threads)
  {
    for (VectorClockContainer::size_type i = 0; i < vc.size(); ++i)
    { // Compare clocks of threads specified in both vector clocks
      if (this->vc[i] > 0) threads.insert(i);
    }
  }

  /**
   * Computes a set of threads in which the operation represented by this vector
   *   clock is not synchronised with an action represented by the specified
   *   vector clock.
   *
   * @param action A vector clock of the action before which the operation
   *   represented by this vector clock should have happened.
   * @param threads A set of threads in which the operation represented by this
   *   vector clock is not synchronised with an action represented by the
   *   @e action vector clock.
   */
  void notHB(const VectorClock_s& action, Threads& threads)
  {
    VectorClockContainer::size_type min = std::min(this->vc.size(), action.vc.size());
    VectorClockContainer::size_type max = std::max(this->vc.size(), action.vc.size());

    for (VectorClockContainer::size_type i = 0; i < min; ++i)
    { // Compare clocks of threads specified in both vector clocks
      if (this->vc[i] > action.vc[i]) threads.insert(i);
    }

    if (action.vc.size() == min)
    { // This action has not clocks for some threads specified
      for (VectorClockContainer::size_type i = min; i < max; ++i)
      { // If we saw the operation in the missing threads, the operation does
        // not need to be executed before the current action, as there is no
        // synchronisation between the thread executing the action and the
        // threads executing the operation
        if (this->vc[i] > 0) threads.insert(i);
      }
    }
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
