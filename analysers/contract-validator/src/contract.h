/**
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-19
 * @version   0.1
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <string>
#include <vector>

#include "fa.hpp"

/**
 * @brief A structure representing a target.
 */
typedef struct Target_s
{
  /**
   * @brief A set of spoilers which may violate the target.
   */
  std::vector< struct Spoiler_s* > spoilers;
} Target;

/**
 * @brief A structure representing a spoiler.
 */
typedef struct Spoiler_s
{
  struct Target_s* target; //!< A target that may be violated by the spoiler.
} Spoiler;

/**
 * @brief A class representing a contract.
 *
 * Represents a contract.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-19
 * @version   0.1
 */
class Contract
{
  private: // Internal data
    /**
     * @brief A set of targets included in the contract.
     */
    std::vector< Target* > m_targets;
  public: // Methods for loading a contract
    void load(const std::string& path);
  private: // Methods for transforming regular expressions to FAs
    FA* construct(const std::string& regex);
};

#endif /* __CONTRACT_H__ */

/** End of file contract.h **/
