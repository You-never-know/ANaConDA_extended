/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains definitions of type-to-type mapping classes.
 *
 * A file containing definitions of classes for mapping objects of one type to
 *   objects of another type.
 *
 * @file      mapper.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-03
 * @date      Last Update 2012-01-27
 * @version   0.1.1
 */

#ifndef __PINTOOL_ANACONDA__MAPPER_H__
  #define __PINTOOL_ANACONDA__MAPPER_H__

#include <map>

#include "pin.H"

/**
 * @brief A class mapping objects of one type to objects of another type.
 *
 * Maps objects of one type to objects of another type.
 *
 * @tparam FROM A type of input objects.
 * @tparam TO A type of output object.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-04
 * @date      Last Update 2011-11-04
 * @version   0.1
 */
template< typename FROM, typename TO >
class Mapper
{
  public: // Destructors
    /**
     * Destroys a Mapper object.
     */
    virtual ~Mapper() {}
  public: // Virtual methods defining the interface for type-to-type conversions
    /**
     * Maps objects of one data type to objects of another data type.
     *
     * @param data The input object which should be mapped.
     * @return The output object to which the input object is mapped.
     */
    virtual TO map(FROM data) = 0;
};

// Type definitions
typedef Mapper< ADDRINT*, const UINT32 > FuncArgMapper;

/**
 * @brief A class for creating objects mapping function arguments to unique IDs.
 *
 * Creates objects mapping function arguments to unique IDs.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-07
 * @date      Last Update 2011-11-07
 * @version   0.1
 */
class FuncArgMapperFactory
{
  public: // Type definitions
    typedef std::map< std::string, FuncArgMapper* > FuncArgMapperMap;
  private: // Static attributes
    static FuncArgMapperFactory* ms_instance; //!< A singleton instance.
  private: // Internal variables
    /**
     * @brief A map containing objects mapping function arguments to unique IDs.
     */
    FuncArgMapperMap m_registeredMappers;
  public: // Static methods
    static FuncArgMapperFactory* Get();
  public: // Member methods for registering and retrieving mapper objects
    FuncArgMapper* getMapper(std::string name);
    void registerMapper(std::string name, FuncArgMapper* mapper);
};

// Macro definitions for simpler mapper registration and retrieval
#define REGISTER_MAPPER(name, mapper) \
  FuncArgMapperFactory::Get()->registerMapper(name, new mapper())
#define GET_MAPPER(name) \
  FuncArgMapperFactory::Get()->getMapper(name)

/**
 * @brief A class mapping addresses to unique IDs.
 *
 * Maps addresses to unique IDs.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-04
 * @date      Last Update 2012-01-27
 * @version   0.1.1
 */
class AddressFuncArgMapper : public FuncArgMapper
{
  public: // Type definitions
    typedef std::map< ADDRINT, const UINT32 > IndexMap;
  private: // Internal variables
    INT32 m_lastIndex; //!< The last unique ID assigned to some address.
    /**
     * @brief A map containing addresses with already assigned unique IDs.
     */
    IndexMap m_indexMap;
    /**
     * @brief A mutex guarding R/W access to both index map and index counter.
     */
    PIN_RWMUTEX m_indexMutex;
  public: // Constructors
    AddressFuncArgMapper();
  public: // Destructors
    virtual ~AddressFuncArgMapper();
  public: // Virtual methods implementing type-to-type conversions
    const UINT32 map(ADDRINT* addr);
};

#endif /* __PINTOOL_ANACONDA__MAPPER_H__ */

/** End of file mapper.h **/
