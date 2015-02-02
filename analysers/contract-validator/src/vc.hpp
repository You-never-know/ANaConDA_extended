/**
 * @brief Contains vector clock implementation.
 *
 * A file containing implementation of a vector clock and its operations.
 *
 * @file      vc.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2015-01-30
 * @date      Last Update 2015-02-02
 * @version   0.1
 */

#ifndef __VC_HPP__
  #define __VC_HPP__

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
  std::vector< vc::clock_t > vc;
} VectorClock;

#endif /* __VC_HPP__ */

/** End of file vc.hpp **/
