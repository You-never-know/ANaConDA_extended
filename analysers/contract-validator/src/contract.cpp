/**
 * @brief Contains implementation of a class representing a contract.
 *
 * A file containing implementation of a class representing a contract.
 *
 * @file      contract.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-21
 * @version   0.1.1
 */

#include "contract.h"

#include <iostream>
#include <regex>

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

// Type definitions
typedef boost::tokenizer< boost::char_separator< char > > Tokenizer;

/**
 * Loads a contract from a file.
 *
 * @param path A path to the file containing the definition of the contract.
 */
void Contract::load(const std::string& path)
{
  if (!fs::exists(path)) return; // Skip non-existent files

  // Helper variables
  fs::fstream f(path);
  std::string line;
  std::smatch mo;

  while (std::getline(f, line) && !f.fail())
  { // Skip all commented and empty lines
    if (line.empty() || line[0] == '#') continue;

    // Format of each line: <target> <- { <spoiler>[, <spoiler>]* }
    std::regex re("^([a-zA-Z0-9_: ]+)[ ]*<-[ ]*\\{[ ]*([a-zA-Z0-9_:, \\|\\(\\)]+)[ ]*\\}[ ]*$");

    // Extract the target and spoiler(s) definitions from the line
    regex_match(line, mo, re);

    // Process the target first
    this->construct(mo[1].str());

    // If more spoilers can violate a target, they are separated by a comma
    Tokenizer spoilers(mo[2].str(), boost::char_separator< char >(","));

    for (Tokenizer::iterator it = spoilers.begin(); it != spoilers.end(); ++it)
    { // Process all spoilers which may violate the target
      this->construct(boost::trim_copy(*it));
    }
  }
}

/**
 * Constructs a finite automaton from a regular expression.
 *
 * @param regex A regular expression.
 * @return A finite automaton representing the regular expression.
 */
FA* Contract::construct(const std::string& regex)
{
  // Helper variables
  FA::State* current = NULL;
  FA::State* epsilon = NULL;
  std::stack< FA::State* > states;
  std::stack< FA::State* > epsilons;

  // Create a new FA
  FA* fa = new FA();
  // Initialise the FA
  fa->start = new FA::State();
  fa->regex = regex;

  // Here we go if we encounter an alternation in the regular expression
  states.push(fa->start);

  // The state we are currently in when performing the transformation
  current = fa->start;

  // Split the regular expression into parts, keep special characters
  Tokenizer tokens(regex, boost::char_separator< char >(" ", "()|"));

  for (Tokenizer::iterator it = tokens.begin(); it != tokens.end(); ++it)
  { // Transform the regular expression into a finite automaton
    switch ((*it).at(0))
    { // Process the current token
      case '(': // Start of a group
        // Save the state marking the beginning of the current group
        states.push(current);
        // Save the epsilon state of the parent group
        epsilons.push(epsilon);
        epsilon = NULL;
        break;
      case ')': // End of a group
        if (epsilon != NULL)
        { // Redirect the end of a possible alternation to the epsilon state
          current->transitions[""] = epsilon;
          // Move to the end of the current group
          current = epsilon;
        }
        // Load the state marking the beginning of the parent group
        states.pop();
        // Load the epsilon state of the parent group
        epsilon = epsilons.top();
        epsilons.pop();
        break;
      case '|': // Alternation
        if (epsilon == NULL)
        { // A state where all alternations end (by taking epsilon transition)
          epsilon = new FA::State();
        } // Redirect the end of a possible alternation to the epsilon state
        current->transitions[""] = epsilon;
        // Move back to the state marking beginning of the current group
        current = states.top();
        break;
      default: // Name of a method
        // Add a new transition for the encountered method in the current state
        current->transitions[boost::trim_copy(*it)] = new FA::State();
        // Advance to the new state by taking the newly added transition
        current = current->transitions[boost::trim_copy(*it)];
        break;
    }
  }

  return fa;
}

/** End of file contract.cpp **/
