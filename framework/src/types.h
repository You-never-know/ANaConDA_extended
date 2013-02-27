/**
 * @brief Contains definitions of types used in various parts of the framework.
 *
 * A file containing definitions of types used in various parts of the
 *   framework.
 *
 * @file      types.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-13
 * @date      Last Update 2013-02-27
 * @version   0.2
 */

#ifndef __PINTOOL_ANACONDA__TYPES_H__
  #define __PINTOOL_ANACONDA__TYPES_H__

#include "pin.H"

// Definitions of classes representing synchronisation primitives
typedef class INDEX< 200 > LOCK; //!< A class representing a lock.
typedef class INDEX< 201 > COND; //!< A class representing a condition.

/**
 * @brief A structure representing a variable.
 */
typedef struct Variable_s
{
  std::string name; //!< A name of the variable.
  std::string type; //!< A type of the variable.
  UINT32 offset; //!< An offset within the variable which was accessed.

  /**
   * Constructs a Variable_s object.
   */
  Variable_s() : name(), type(), offset(0) {}

  /**
   * Constructs a Variable_s object.
   *
   * @param n A name of a variable.
   * @param t A type of a variable.
   * @param o An offset within a variable which was accessed.
   */
  Variable_s(const std::string& n, const std::string& t, const UINT32 o)
    : name(n), type(t), offset(o) {}
} VARIABLE;

/**
 * @brief A structure representing a source code location.
 */
typedef struct Location_s
{
  std::string file; //!< A name of a file.
  INT32 line; //!< A line number.

  /**
   * Constructs a Location_s object.
   */
  Location_s() : file(), line(-1) {}
} LOCATION;

#endif /* __PINTOOL_ANACONDA__TYPES_H__ */

/** End of file types.h **/
