/**
 * @brief Contains definitions of classes representing program analysers.
 *
 * A file containing definitions of classes representing program analysers.
 *
 * @file      analyser.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-01-06
 * @version   0.1.2.1
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
 * @date      Last Update 2012-01-06
 * @version   0.1.2.1
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
  public: // Member methods for interacting with the analyser
    void init();
  public: // Inline member methods
    /**
     * Gets a path to a program analyser's library.
     *
     * @return The path to the program analyser's library.
     */
    const fs::path& getLibraryPath()
    {
      return m_shlib->getPath();
    }

    /**
     * Gets an address at which is a program analyser's library loaded.
     *
     * @return The address at which is the program analyser's library loaded or
     *   @em NULL if the address could not be resolved.
     */
    void* getLibraryAddress()
    {
      return m_shlib->getAddress();
    }
};

#endif /* __PINTOOL_ANACONDA__ANALYSER_H__ */

/** End of file analyser.h **/
