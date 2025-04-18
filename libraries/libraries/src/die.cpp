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
 * @brief A file containing implementation of classes for extracting, holding
 *   and manipulating the debugging information.
 *
 * A file containing implementation of classes for extracting, holding and
 *   manipulating the debugging information.
 *
 * @file      die.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-01
 * @date      Last Update 2012-05-24
 * @version   0.1.0.2
 */

#include "die.h"

#ifdef TARGET_LINUX
  #include "dwarf/dw_die.h"
#endif

/**
 * Constructs a DebugInfo object.
 */
DebugInfo::DebugInfo()
{
}

/**
 * Destroys a DebugInfo object.
 */
DebugInfo::~DebugInfo()
{
}

/**
 * Constructs an ExtractionError object.
 *
 * @param message A message describing an error.
 */
ExtractionError::ExtractionError(std::string message) throw()
  : m_message(message)
{
}

/**
 * Constructs an ExtractionError object from an existing ExtractionError object.
 *
 * @param ee An ExtractionError object.
 */
ExtractionError::ExtractionError(const ExtractionError& ee) throw()
  : m_message(ee.m_message)
{
}

/**
 * Destroys a ExtractionError object.
 */
ExtractionError::~ExtractionError() throw()
{
}

/**
 * Gets the debugging information from a file.
 *
 * @param filename A name of the file.
 * @return An object containing the debugging information.
 */
DebugInfo* DIE_GetDebugInfo(std::string filename)
{
#ifdef TARGET_LINUX
  return DwarfDebugInfoExtractor::Get()->getDebugInfo(filename);
#else
  return NULL;
#endif
}

/** End of file die.cpp **/
