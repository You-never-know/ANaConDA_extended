/**
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-27
 * @version   0.1
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <string>

/**
 * @brief A class representing a contract.
 *
 * Represents a contract.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-27
 * @version   0.1
 */
class Contract
{
  private: // Internal data
    //
  public: // Methods for loading contracts
    void load(std::string path);
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
