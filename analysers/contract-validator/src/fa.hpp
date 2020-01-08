/*
 * Copyright (C) 2016-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of a class representing a finite automaton.
 *
 * A file containing implementation of a class representing a finite automaton.
 *
 * @file      fa.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-28
 * @version   0.5
 */

#ifndef __FA_HPP__
  #define __FA_HPP__

#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <ostream>

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
 * @date      Last Update 2016-02-28
 * @version   0.3
 */
template< class FA >
class BasicFARunner
{
  public: // Type definitions
    /**
     * @brief An enumeration representing the results of the advance() method.
     */
    enum
    {
      MOVED_TO_NEXT_STATE, //!< A finite automaton moved to the next state.
      NO_TRANSITION_FOUND, //!< No transition found for the symbol specified.
      INVALID_SYMBOL       //!< The symbol does not belong to the alphabet.
    };
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
     * Advances the finite automaton to the next state.
     *
     * @param symbol A name of a symbol encountered in the execution.
     * @return @c MOVED_TO_NEXT_STATE if the finite automaton advanced to the
     *   next state successfully, @c NO_TRANSITION_FOUND if the automaton was
     *   not able to advance because there is no transition for a symbol from
     *   its alphabet, @c INVALID_SYMBOL if the automaton could not advance
     *   because the symbol is not from its alphabet.
     */
    int advance(const std::string& symbol)
    {
      // Ignore all symbols not belonging to the alphabet
      if (m_fa->alphabet.count(symbol) == 0) return INVALID_SYMBOL;

      try
      { // Try to advance the automaton to the next state
        m_current = m_current->transitions.at(symbol);

        // Transition containing the specified symbol was taken
        return MOVED_TO_NEXT_STATE;
      }
      catch (std::out_of_range& e)
      { // No transition containing the specified symbol found
        return NO_TRANSITION_FOUND;
      }
    }

    /**
     * Resets the finite automaton to the starting state.
     */
    void reset()
    {
      m_current = m_fa->start;
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

/**
 * Prints a finite automaton to a stream.
 *
 * @param s A stream to which the finite should be printed.
 * @param value A structure representing the finite automaton.
 * @return The stream to which was the finite automaton printed.
 */
inline
std::ostream& operator<<(std::ostream& s, const FA& fa)
{
  // Helper variables
  FA::State* current;
  std::set< FA::State* > visited; // States already processed or scheduled
  std::list< FA::State* > states; // States scheduled to be processed
  std::map< std::string, FAState_s* >::iterator it;

  // Search all states from the starting state
  states.push_back(fa.start);

  // Basic information about the finite automata
  s << "FA " << fa.regex << "\n";

  while (!states.empty())
  { // Take the first state not visited yet and process it
    current = states.front();
    states.pop_front();

    // Print information about the state itself
    s << "State " << std::hex << current << ((current->accepting)
      ? " [accepting]" : "") << "\n";

    for (it = current->transitions.begin(); it != current->transitions.end();
      ++it)
    { // Print information about each transition from the current state
      s << it->first << "->" << std::hex << it->second << "\n";

      if (visited.count(it->second) == 0)
      { // Schedule the state where the transition leads for processing
        states.push_back(it->second);
        // Mark this state as visited as we now know we will process it
        visited.insert(it->second);
      }
    }
  }

  return s;
}

/**
 * Concatenates a string with a finite automaton.
 *
 * @param s A string.
 * @param fa A finite automaton.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em fa.
 */
inline
std::string operator+(const std::string& s, const FA& fa)
{
  // Print the FA to a stream, then convert the stream to a string
  std::stringstream ss;

  ss << fa;

  return s + ss.str();
}

/** End of file fa.hpp **/
