/**
 * @brief Contains definitions of classes for writing data.
 *
 * A file containing definitions of classes for writing data.
 *
 * @file      writers.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-07
 * @date      Last Update 2013-05-30
 * @version   0.1.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__WRITERS_H__
  #define __PINTOOL_ANACONDA__UTILS__WRITERS_H__

#include <fstream>

/**
 * @brief A class for writing data to a file.
 *
 * Writes data to a file.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-07
 * @date      Last Update 2013-02-07
 * @version   0.1
 */
class FileWriter
{
  private: // Internal variables
    std::fstream m_file; //!< A file to which will the data be written.
  public: // Control methods
    void open(const std::string& path);
    void close();
  protected: // Write methods
    void write(const std::string& data);
    void writeln(const std::string& data);
};

#endif /* __PINTOOL_ANACONDA__UTILS__WRITERS_H__ */

/** End of file writers.h **/
