/*
 * Copyright (C) 2014-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definition of a class representing a contract.
 *
 * A file containing definition of a class representing a contract.
 *
 * @file      contract.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2019-02-06
 * @version   0.6.1
 */

#ifndef __CONTRACT_H__
  #define __CONTRACT_H__

#include <list>
#include <string>

#include "anaconda/utils/scopedlock.hpp"

#include "../../simple-contract-validator/src/fa.hpp"

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
