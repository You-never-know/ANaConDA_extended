/**
 * @brief Contains implementation of templates for generating sequences.
 *
 * A file containing implementation of templates for generating sequences.
 *
 * @file      seq.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-15
 * @date      Last Update 2013-02-15
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__PIN__SEQ_HPP__
  #define __PINTOOL_ANACONDA__PIN__SEQ_HPP__

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

#endif /* __PINTOOL_ANACONDA__PIN__SEQ_HPP__ */

/** End of file seq.hpp **/
