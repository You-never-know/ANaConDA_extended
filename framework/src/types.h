/**
 * @brief Contains definitions of types used in various parts of the framework.
 *
 * A file containing definitions of types used in various parts of the framework
 *   together with some utility functions for their formatting or serialisation.
 *
 * @file      types.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-13
 * @date      Last Update 2013-09-24
 * @version   0.3.1.1
 */

#ifndef __PINTOOL_ANACONDA__TYPES_H__
  #define __PINTOOL_ANACONDA__TYPES_H__

#include <ostream>

#include "pin.H"

// Definitions of classes representing synchronisation primitives
typedef class INDEX< 200 > LOCK; //!< A class representing a lock.
typedef class INDEX< 201 > COND; //!< A class representing a condition.
typedef class INDEX< 202 > THREAD; //!< A class representing a thread.

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

/**
 * Prints a lock object to a stream.
 *
 * @param s A stream to which the lock object should be printed.
 * @param value A lock object.
 * @return The stream to which was the lock object printed.
 */
inline
std::ostream& operator<<(std::ostream& s, const LOCK& value)
{
  return s << "LOCK(index=" << value.q() << ")";
}

/**
 * Prints a condition object to a stream.
 *
 * @param s A stream to which the condition object should be printed.
 * @param value A condition object.
 * @return The stream to which was the condition object printed.
 */
inline
std::ostream& operator<<(std::ostream& s, const COND& value)
{
  return s << "COND(index=" << value.q() << ")";
}

/**
 * Concatenates a string with a lock object.
 *
 * @param s A string.
 * @param lock A lock object.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em lock.
 */
inline
std::string operator+(const std::string& s, const LOCK& lock)
{
  return s + "LOCK(index=" + decstr(lock.q()) + ")";
}

/**
 * Concatenates a lock object with a string.
 *
 * @param lock A lock object.
 * @param s A string
 * @return A new string with a value of a string representation of @em lock
 *   followed by @em s.
 */
inline
std::string operator+(const LOCK& lock, const std::string& s)
{
  return "LOCK(index=" + decstr(lock.q()) + ")" + s;
}

/**
 * Concatenates a string with a condition object.
 *
 * @param s A string.
 * @param cond A condition object.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em cond.
 */
inline
std::string operator+(const std::string& s, const COND& cond)
{
  return s + "COND(index=" + decstr(cond.q()) + ")";
}

/**
 * Concatenates a condition object with a string.
 *
 * @param cond A condition object.
 * @param s A string.
 * @return A new string with a value of a string representation of @em cond
 *   followed by @em s.
 */
inline
std::string operator+(const COND& cond, const std::string& s)
{
  return "COND(index=" + decstr(cond.q()) + ")" + s;
}

#endif /* __PINTOOL_ANACONDA__TYPES_H__ */

/** End of file types.h **/
