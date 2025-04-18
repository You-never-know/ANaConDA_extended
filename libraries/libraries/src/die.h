/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of libdie.
 *
 * libdie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libdie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdie. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing definitions of classes for extracting, holding and
 *   manipulating the debugging information.
 *
 * A file containing definitions of classes for extracting, holding and
 *   manipulating the debugging information.
 *
 * @file      die.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-01
 * @date      Last Update 2011-09-15
 * @version   0.1.0.1
 */

#ifndef __LIBDIE__DIE_H__
  #define __LIBDIE__DIE_H__

#include <exception>
#include <string>

/**
 * @brief An interface defining methods for accessing the debugging information.
 *
 * Defines methods for accessing various debugging information.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-09-15
 * @version   0.1.1
 */
class DebugInfo
{
  protected: // Constructors
    DebugInfo();
  public: // Destructors
    virtual ~DebugInfo();
  public: // Virtual methods
    virtual void printDebugInfo() = 0;
};

/**
 * @brief A class representing an error during debugging information extraction.
 *
 * Represents an error which occurred when extracting the debugging information
 *   from a file.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-09-15
 * @version   0.1.0.1
 */
class ExtractionError : public std::exception
{
  protected: // Internal variables
    std::string m_message; //!< A message describing the error.
  public: // Constructors
    ExtractionError(std::string message) throw();
    ExtractionError(const ExtractionError& ee) throw();
  public: // Destructors
    virtual ~ExtractionError() throw();
  public: // Virtual methods for accessing error messages
    /**
     * Gets a C-style character string describing an error.
     *
     * @return A C-style character string describing the error.
     */
    virtual const char* what() const throw() { return this->m_message.c_str(); }
};

DebugInfo* DIE_GetDebugInfo(std::string filename);

#endif /* __LIBDIE__DIE_H__ */

/** End of file die.h **/
