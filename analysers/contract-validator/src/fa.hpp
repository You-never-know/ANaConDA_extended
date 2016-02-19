/**
 * @brief Contains implementation of a class representing a finite automaton.
 *
 * A file containing implementation of a class representing a finite automaton.
 *
 * @file      fa.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-19
 * @version   0.1.1
 */

#ifndef __FA_HPP__
  #define __FA_HPP__

#include <map>
#include <set>
#include <stdexcept>

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

  /**
   * Constructs a new non-accepting state of a finite automaton (FA).
   */
  FAState_s() : accepting(false) {}

  /**
   * Constructs a new state of a finite automaton (FA).
   *
   * @param a A flag determining if the state is accepting or not. If set to @c
   *   true, the state will be an accepting state. Otherwise, the state will be
   *   a non-accepting state.
   */
  FAState_s(bool a) : accepting(a) {}
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
  std::string regex; //!< A regular expression accepted by this FA.
  std::set< std::string > alphabet; //!< A set of symbols accepted by this FA.
};

/**
 * @brief A class representing a single run of a finite automaton (FA).
 *
 * Represents a single run of a finite automaton (FA).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-18
 * @version   0.1
 */
template< class FA >
class BasicFARunner
{
  private: // Internal data
    FA* m_fa; //!< The finite automaton whose run this class controls.
    typename FA::State* m_current; //!< Current state of the finite automaton.
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
    bool advance(const std::string& symbol)
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
     * Gets a regular expression accepted by the automaton.
     *
     * @return A regular expression accepted by the automaton.
     */
    const std::string& regex()
    {
      return m_fa->regex;
    }
};

#endif /* __FA_HPP__ */

// Definitions of finite automata and their runs which should be used
typedef SimpleFA< FAState > FA;
typedef BasicFARunner< FA > FARunner;

/** End of file fa.hpp **/
