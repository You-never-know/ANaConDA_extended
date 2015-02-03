/**
 * @brief Contains implementation of a class representing a finite automaton.
 *
 * A file containing implementation of a class representing a finite automaton.
 *
 * @file      fa.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2015-02-03
 * @version   0.9.1
 */

#ifndef __FA_HPP__
  #define __FA_HPP__

#include "anaconda.h"

#include <map>
#include <set>
#include <stdexcept>
#include <string>

/**
 * @brief A structure representing a state of a finite automaton (FA).
 */
typedef struct FAState_s
{
  /**
   * @brief A list of transitions that can be taken from the state.
   */
  std::map< std::string, FAState_s* > transitions;
  bool accepting; //!< A flag determining if the state is accepting or not.
  std::string sequence; //!< A symbol sequence needed to get to this state.
  unsigned int id; //!< A number identifying the state.

  /**
   * Constructs a new non-accepting state of a finite automaton (FA).
   */
  FAState_s() : accepting(false), id(0) {}

  /**
   * Constructs a new state of a finite automaton (FA).
   *
   * @param a A flag determining if the state is accepting or not. If set to @c
   *   true, the state will be an accepting state. Otherwise, the state will be
   *   a non-accepting state.
   */
  FAState_s(bool a) : accepting(a), id(0) {}
} FAState;

/**
 * @brief A structure representing a simple finite automaton (FA).
 *
 * @tparam S A structure representing the states of the finite automaton.
 */
template< typename S >
struct SimpleFA
{
  /**
   * @brief A structure representing the states of the finite automaton.
   */
  typedef S State;

  State* start; //!< A starting state.
  std::set< std::string > alphabet; //!< A set of symbols accepted by this FA.
};

/**
 * @brief A class representing a single run of a finite automaton (FA).
 *
 * Represents a single run of a finite automaton (FA).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-28
 * @date      Last Update 2015-02-03
 * @version   0.3
 */
template< class FA >
class BasicFARunner
{
  private: // Internal data
    FA* m_fa; //!< The finite automaton whose run this class controls.
    typename FA::State* m_current; //!< Current state of the finite automaton.
  public: // Additional data assigned to the class
    std::set< LOCK > lockset; //!< A set containing all locks held by a thread.
  public: // Constructors
    /**
     * Constructs a class representing a single run of a finite automaton (FA).
     *
     * @param fa A finite automaton whose run will be controlled.
     */
    BasicFARunner(FA* fa) : m_fa(fa), m_current(fa->start) {}
  public: // Automaton manipulation methods
    /**
     * Advances the finite automaton to a next state.
     *
     * @note If the symbol does not belong to the alphabet of the finite
     *   automaton, the finite automaton advances to the current state.
     *
     * @param symbol A name of a symbol encountered in the execution.
     * @return @em True if the finite automaton advanced to a next state,
     *   @em false otherwise.
     */
    bool advance(std::string symbol)
    {
      // Ignore all symbols not belonging to the alphabet
      if (m_fa->alphabet.count(symbol) == 0) return true;

      try
      { // Try to advance the automaton to the next state
        m_current = m_current->transitions.at(symbol);

        return true; // Transition containing the specified symbol was taken
      }
      catch (std::out_of_range& e)
      { // No transition containing the specified symbol found
        return false;
      }
    }

    /**
     * Checks if the finite automaton accepted the symbol sequence.
     *
     * @return @em True if the finite automaton accepted the symbol sequence,
     *   @em false otherwise.
     */
    bool accepted()
    {
      return m_current->accepting; // Check if the state is an accepting state
    }

    /**
     * Gets a symbol sequence which was accepted by the automaton.
     *
     * @return A symbol sequence which was accepted by the automaton.
     */
    const std::string& sequence()
    {
      return m_current->sequence;
    }

    /**
     * Gets the current state.
     *
     * @return The current state.
     */
    const typename FA::State* state()
    {
      return m_current;
    }
};

// Definitions of finite automata and their runs which should be used
typedef SimpleFA< FAState > FA;
typedef BasicFARunner< FA > FARunner;

#endif /* __FA_HPP__ */

/** End of file fa.hpp **/
