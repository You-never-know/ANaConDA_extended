/*
 * Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of classes for writing data.
 *
 * A file containing implementation of classes for writing data.
 *
 * @file      writers.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-07
 * @date      Last Update 2015-06-01
 * @version   0.1.1
 */

#include "writers.h"

/**
 * Opens a file.
 *
 * @param path A path to the file.
 */
void FileWriter::open(const std::string& path)
{
  m_file.open(path, std::fstream::out | std::fstream::trunc);
}

/**
 * Closes a file.
 */
void FileWriter::close()
{
  m_file.close();
}

/**
 * Writes data to a file.
 *
 * @param data The data.
 */
void FileWriter::write(const std::string& data)
{
  m_file << data.c_str();
}

/**
 * Writes a line of data to a file.
 *
 * @param data The data.
 */
void FileWriter::writeln(const std::string& data)
{
  m_file << data.c_str() << "\n";
}

/** End of file writers.cpp **/
