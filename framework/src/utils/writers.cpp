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
