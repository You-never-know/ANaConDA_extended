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
 * @brief Contains implementation of a class representing a contract.
 *
 * A file containing implementation of a class representing a contract.
 *
 * @file      contract.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2015-02-03
 * @version   0.6
 */

#include "../../simple-contract-validator/src/contract.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

namespace
{ // Internal type definitions and variables (usable only within this module)
  unsigned int g_currId = 0;
}

/**
 * Loads a contract from a file.
 *
 * @param path A path to a file containing the definition of a contract.
 */
void Contract::load(std::string path)
{
  if (fs::exists(path))
  { // Extract all method sequences from a contract specification file
    fs::fstream f(path);

    // Helper variables
    std::string line;

    // Helper classes
    class MethodSequence
    { // Splits a method sequence into separate parts (concrete methods)
      private: // Internal variables for storing methods in the sequence
        typedef boost::tokenizer< boost::char_separator< char > > Tokenizer;
        Tokenizer m_parts; // Container for storing methods in the sequence
        Tokenizer::iterator m_it; // Pointer to the currently processed method
      public: // Process a method sequence, methods are separated by spaces
        MethodSequence(std::string& l) : m_parts(l, boost::char_separator
          < char >(" ")), m_it(m_parts.begin()) {};
      public: // Methods for checking and accessing methods in the sequence
        bool hasMoreParts() { return m_it != m_parts.end(); }
        std::string nextPart() { return *m_it++; }
    };

    // Construct a new FA with a start state without any transitions
    m_sequences = new FA();
    m_sequences->start = new FA::State();
    m_violations = new FA();
    m_violations->start = new FA::State();

    // Helper variables
    std::string method;
    std::string sequence;
    FA* fa = NULL; // Currently constructed FA
    FA::State* state = NULL; // Actual state in the currently constructed FA
    FA::State* start = NULL; // Start state of the currently constructed FA
    FA::State* accepting = NULL; // Accepting state of the contract sequence FA

    while (std::getline(f, line) && !f.fail())
    { // Skip all commented and empty lines
      if (line.empty() || line[0] == '#') continue;
      // Process a method sequence
      MethodSequence ms(line);

      // Transform the sequence to FA, begin from the start state
      state = (fa = m_sequences)->start;
      accepting = NULL;

      while (ms.hasMoreParts())
      { // Insert the methods as transitions of the current state
        if ((method = ms.nextPart()) == "<-")
        { // End of current method sequence, last state is accepting
          state->accepting = true;
          state->sequence = sequence;

          // Assign an ID to the accepting state as we need to store VC for it
          if (state->id == 0) state->id = ++g_currId;

          // We are moving to the next sequence
          sequence.clear();

          state->start = start; // Save reference to the start state

          if (accepting == NULL)
          { // This is the accepting state of the contract sequence FA
            accepting = state;
          }
          else
          { // This is the accepting state of the violation sequence FA
            accepting->conflicts.insert(state);
            state->conflicts.insert(accepting);
          }

          // Next sequence is a sequence which may violate the contract
          state = (fa = m_violations)->start;

          continue; // Ignore the delimiter, move to the next method
        }

        // We are processing a method from some sequence
        fa->alphabet.insert(method);

        try
        { // If there is already a transition for the method, advance
          state = state->transitions.at(method);
        }
        catch (std::out_of_range& e)
        { // If there is no transition, create a new one
          state->transitions[method] = new FA::State();
          // And move to the new state
          state = state->transitions[method];
        }

        if (sequence.empty())
        { // This is the state reached after encountering the first method of
          // a sequence, assign an ID to it as we need to store VC for it
          if (state->id == 0) state->id = ++g_currId;

          start = state; // This is the start state of the current sequence
        }

        // Add the currently processed method to currently processed sequence
        sequence += " " + method;
      }

      // The state where we ended is the accepting state
      state->accepting = true;
      state->sequence = sequence;

      // Assign an ID to the accepting state as we need to store VC for it
      if (state->id == 0) state->id = ++g_currId;

      state->start = start; // Save reference to the start state

      if (accepting == NULL)
      { // This is the accepting state of the contract sequence FA
        accepting = state;
      }
      else
      { // This is the accepting state of the violation sequence FA
        accepting->conflicts.insert(state);
        state->conflicts.insert(accepting);
      }

      // We are moving to the next sequence
      sequence.clear();
    }
  }
}

/**
 * Checks is some method sequence of the contract begins with a function with a
 *   specific name.
 *
 * @param function A name of the function currently being executed in a program.
 */
FARunner* Contract::startsWith(std::string function)
{
  // Prevent deadlocks when calling std::map.at() in PIN's analysis functions
  ScopedLock lock(m_contractLock);

  try
  { // If we can make a transition from the start state, some sequence begins
    // with the function of the name specified
    m_sequences->start->transitions.at(function);

    // Transition exists, we have to check this contract
    FARunner* cc = new FARunner(m_sequences);

    // Remember that we are checking this contract somewhere
    m_checked.push_back(cc);

    return cc;
  }
  catch (std::out_of_range& e)
  { // No transition could be taken, no sequence begins with this function
    return NULL;
  }
}

/**
 * Checks is some violation sequence of the contract begins with a function with
 *   a specific name.
 *
 * @param function A name of the function currently being executed in a program.
 */
FARunner* Contract::violationStartsWith(std::string function)
{
  // Prevent deadlocks when calling std::map.at() in PIN's analysis functions
  ScopedLock lock(m_contractLock);

  try
  { // If we can make a transition from the start state, some sequence begins
    // with the function of the name specified
    m_violations->start->transitions.at(function);

    // Transition exists, we have to check this violation
    FARunner* cv = new FARunner(m_violations);

    // Remember that we are checking this violation somewhere
    m_checked.push_back(cv);

    return cv;
  }
  catch (std::out_of_range& e)
  { // No transition could be taken, no sequence begins with this function
    return NULL;
  }
}

/** End of file contract.cpp **/
