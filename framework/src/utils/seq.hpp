/*
 * Copyright (C) 2013-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of templates for generating sequences.
 *
 * A file containing implementation of templates for generating sequences.
 *
 * @file      seq.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-15
 * @date      Last Update 2013-05-30
 * @version   0.1.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__SEQ_HPP__
  #define __PINTOOL_ANACONDA__UTILS__SEQ_HPP__

/**
 * @brief A structure representing an integer sequence.
 */
template< int... >
struct seq {};

/**
 * @brief A structure for generating ascending integer sequences.
 */
template< int N, int... S >
struct gens : gens< N - 1, N - 1, S... > {};

/**
 * @brief A structure stopping the generation of ascending integer sequence.
 */
template< int... S >
struct gens< 0, S... >
{
  typedef seq< S... > type;
};

#endif /* __PINTOOL_ANACONDA__UTILS__SEQ_HPP__ */

/** End of file seq.hpp **/
