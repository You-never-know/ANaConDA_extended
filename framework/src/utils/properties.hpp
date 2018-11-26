/*
 * Copyright (C) 2013-2018 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of a class for storing properties.
 *
 * A file containing implementation of a class for storing properties (key/value
 *   pairs). The key is always a string, however, the value might be of any data
 *   type and the class ensures it is stored and retrieved properly.
 *
 * @file      properties.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-20
 * @date      Last Update 2013-05-30
 * @version   0.1.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__PROPERTIES_HPP__
  #define __PINTOOL_ANACONDA__UTILS__PROPERTIES_HPP__

#include <map>

#include <boost/any.hpp>

/**
 * @brief A container for storing properties.
 *
 * Stores properties (key/value pairs) and provides access to them. The values
 *   of properties might be of any data type.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-05-20
 * @date      Last Update 2013-05-21
 * @version   0.1
 */
class Properties
{
  private: // Type definitions
    typedef std::map< std::string, boost::any > PropertyMap;
  private: // Internal variables
    PropertyMap m_properties; //!< An underlying map containing the properties.
  public: // Inline template methods
    /**
     * Gets a value of a property.
     *
     * @param key A string identifying the property.
     * @return The value of the property.
     */
    template< typename T >
    const T& get(const std::string& key)
    {
      // Use any_cast with pointers or else we get a copy we cannot reference
      return *boost::any_cast< T >(&m_properties[key]);
    }

    /**
     * Sets a value of a property.
     *
     * @param key A string identifying the property.
     * @param value A value of the property.
     */
    template< typename T >
    void set(const std::string& key, const T& value)
    {
      m_properties.insert(make_pair(key, value));
    }

    /**
     * Checks if the container contains a property.
     *
     * @param key A string identifying the property.
     * @return @em True if the container contains the property, @em false
     *   otherwise.
     */
    bool contains(const std::string& key)
    {
      return m_properties.count(key) != 0;
    }
};

#endif /* __PINTOOL_ANACONDA__UTILS__PROPERTIES_HPP__ */

/** End of file properties.hpp **/
