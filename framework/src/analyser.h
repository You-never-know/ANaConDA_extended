/**
 * @brief Contains definitions of classes representing program analysers.
 *
 * A file containing definitions of classes representing program analysers.
 *
 * @file      analyser.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2011-12-09
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__ANALYSER_H__
  #define __PINTOOL_ANACONDA__ANALYSER_H__

#include "shlib.h"

/**
 * @brief A class representing a program analyser.
 *
 * Represents a program analyser.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2011-12-09
 * @version   0.1
 */
class Analyser
{
  private: // Internal variables
    SharedLibrary* m_shlib; //!< A shared library containing a program analyser.
  public: // Static methods
    static Analyser* Load(fs::path path, std::string& error);
  private: // Internal constructors
    Analyser(SharedLibrary* shlib);
  public: // Constructors
    Analyser(const Analyser& a);
  public: // Destructors
    ~Analyser();
};

#endif /* __PINTOOL_ANACONDA__ANALYSER_H__ */

/** End of file analyser.h **/
