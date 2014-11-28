/**
 * @brief Contains implementation of a class representing a finite automaton.
 *
 * A file containing implementation of a class representing a finite automaton.
 *
 * @file      fa.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-28
 * @version   0.3.1
 */

#ifndef __FA_HPP__
  #define __FA_HPP__

#include <map>
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
 * @brief A structure representing a finite automaton (FA).
 *
 * @tparam S A class or structure representing states of the finite automaton.
 */
template< typename S >
typedef struct FA_s
{
  /**
   * @brief A class or structure representing states of the finite automaton.
   */
  typedef typename S State;

  State* start; //!< A starting state.
} FA;

/**
 * @brief A class representing a single run of a finite automaton (FA).
 *
 * Represents a single run of a finite automaton (FA).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-28
 * @date      Last Update 2014-11-28
 * @version   0.1
 */
template< typename FA >
class FARunner
{
  private:
    FA* m_fa; //!< The finite automaton whose run is described.
    typename FA::State* m_current; //!< Current state of the finite automaton.
  public:
    /**
     * Advances the finite automaton to the next state.
     *
     * @param symbol A name of a symbol encountered in the execution.
     * @return @em True if the finite automaton advanced to some next state,
     *   @em false otherwise.
     */
    bool advance(std::string symbol)
    {
      try
      { // Try to advance the automaton to the next state
        m_current = m_current->transitions.at(symbol);

        return true; // Transition containing the specified symbol was taken
      }
      catch (std::out_of_range& e)
      { // No transition containing the specified symbol
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
};

#endif /* __FA_HPP__ */

/** End of file fa.hpp **/
