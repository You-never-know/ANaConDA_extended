/**
 * @brief Contains implementation of a class representing a contract.
 *
 * A file containing implementation of a class representing a contract.
 *
 * @file      contract.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2015-02-01
 * @version   0.5
 */

#include "contract.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

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

    while (std::getline(f, line) && !f.fail())
    { // Skip all commented and empty lines
      if (line.empty() || line[0] == '#') continue;
      // Process a method sequence
      MethodSequence ms(line);

      // Transform the sequence to FA, begin from the start state
      FA::State* state = m_sequences->start;

      while (ms.hasMoreParts())
      { // Insert the methods as transitions of the current state
        std::string method = ms.nextPart();

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
      }

      // The state where we ended is the accepting state
      state->accepting = true;
      state->sequence = line;
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

/** End of file contract.cpp **/
