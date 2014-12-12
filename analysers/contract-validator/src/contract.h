/**
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-12-12
 * @version   0.3
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <list>
#include <string>

#include "fa.hpp"

/**
 * @brief A class representing a contract.
 *
 * Represents a contract.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-28
 * @version   0.2
 */
class Contract
{
  private: // Internal data
    FA* m_definition; //!< The definition of the contract.
    std::list< FARunner* > m_checked; //!< Currently checked contract instances.
  public: // Methods for loading contracts
    void load(std::string path);
  public: // Methods for checking contracts
    FARunner* startsWith(std::string function);
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
