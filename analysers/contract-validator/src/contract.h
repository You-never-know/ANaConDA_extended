/**
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2015-02-03
 * @version   0.6
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <list>
#include <string>

#include "utils/scopedlock.hpp"

#include "fa.hpp"

/**
 * @brief A class representing a contract.
 *
 * Represents a contract.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2015-02-03
 * @version   0.6
 */
class Contract
{
  private: // Internal data
    /**
     * @brief An encoded set of (method) sequences representing a contract.
     */
    FA* m_sequences;
    /**
     * @brief An encoded set of (method) sequences that may violate a contract.
     */
    FA* m_violations;
    std::list< FARunner* > m_checked; //!< Currently checked contract instances.
    PIN_MUTEX m_contractLock; //!< A lock guarding access to internal data.
  public: // Constructors
    Contract() : m_sequences(NULL), m_violations(NULL)
    {
      PIN_MutexInit(&m_contractLock);
    }
  public: // Destructors
    ~Contract() { PIN_MutexFini(&m_contractLock); }
  public: // Methods for loading contracts
    void load(std::string path);
  public: // Methods for checking contracts
    FARunner* startsWith(std::string function);
    FARunner* violationStartsWith(std::string function);
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
