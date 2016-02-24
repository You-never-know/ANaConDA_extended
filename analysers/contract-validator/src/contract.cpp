/**
 * @brief Contains implementation of a class representing a contract.
 *
 * A file containing implementation of a class representing a contract.
 *
 * @file      contract.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-24
 * @version   0.5.2
 */

#include "contract.h"

#include <iostream>
#include <regex>
#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

namespace
{ // Internal type definitions and variables (usable only within this module)
  Target::Type g_currTargetType = 0;
  Spoiler::Type g_currSpoilerType = 0;
}

// Type definitions
typedef boost::tokenizer< boost::char_separator< char > > Tokenizer;

/**
 * Destroys a contract freeing all of its targets and spoilers.
 */
Contract::~Contract()
{
  for (Target* target : m_targets)
  { // Free all targets which the contract contains
    for (Spoiler* spoiler : target->spoilers)
    { // Free all spoilers that can violate the target
      delete spoiler->fa;
      delete spoiler;
    }

    // Free the target after freeing all of its spoilers
    delete target->fa;
    delete target;
  }
}

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
  Target* target = NULL;
  Spoiler* spoiler = NULL;

  while (std::getline(f, line) && !f.fail())
  { // Skip all commented and empty lines
    if (line.empty() || line[0] == '#') continue;

    // Format of each line: <target> <- { <spoiler>[, <spoiler>]* }
    std::regex re("^([a-zA-Z0-9_: ]+)[ ]*<-[ ]*\\{[ ]*([a-zA-Z0-9_:, \\|\\(\\)]+)[ ]*\\}[ ]*$");

    // Extract the target and spoiler(s) definitions from the line
    regex_match(line, mo, re);

    // Process the target first
    target = new Target(g_currTargetType++);

    // Create a FA which represents the target
    target->fa = this->construct(mo[1].str());

    // If more spoilers can violate a target, they are separated by a comma
    Tokenizer spoilers(mo[2].str(), boost::char_separator< char >(","));

    for (Tokenizer::iterator it = spoilers.begin(); it != spoilers.end(); ++it)
    { // Process all spoilers which may violate the target
      spoiler = new Spoiler(g_currSpoilerType++);

      // Create a FA which represents the spoiler
      spoiler->fa = this->construct(boost::trim_copy(*it));

      // Link the spoiler with the target it can violate
      target->spoilers.push_back(spoiler);
      spoiler->target = target;
    }

    // Assign the target to the contract
    m_targets.push_back(target);
  }
}

/**
 * Creates a string representation of a contract.
 *
 * @return A string representation of the contract.
 */
std::string Contract::toString()
{
  // Helper variables
  std::stringstream ss;

  // Basic information about a contract
  ss << "Contract " << std::hex << this << "\n";

  for (Target* target : m_targets)
  { // Convert all targets to a string
    ss << "  Target " << target << "\n" << *target->fa;

    for (Spoiler* spoiler : target->spoilers)
    { // Convert all spoilers which may violated a target to a string
      ss << "    Spoiler " << spoiler << "\n" << *spoiler->fa;
    }
  }

  return ss.str();
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
  fa->regex = boost::trim_copy(regex);

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
        // TODO: This should be shared by all target/spoilers in a contract
        fa->alphabet.insert(boost::trim_copy(*it));
        // Add a new transition for the encountered method in the current state
        current->transitions[boost::trim_copy(*it)] = new FA::State();
        // Advance to the new state by taking the newly added transition
        current = current->transitions[boost::trim_copy(*it)];
        break;
    }
  }

  // Flag the last state in which we ended as accepting
  current->accepting = true;

  return this->toEpsilonFreeFA(fa);
}

/**
 * Transforms a finite automaton which may contain epsilon transitions into a
 *   finite automaton without epsilon transitions.
 *
 * @param fa A finite automaton which may contain epsilon transitions.
 * @return A finite automaton without epsilon transitions.
 */
FA* Contract::toEpsilonFreeFA(FA* fa)
{
  // Helper variables
  FA::State* target = NULL;
  FA::State* current = NULL;
  FA::State* epsilon = NULL;
  std::set< FA::State* > visited; // States already processed or scheduled
  std::list< FA::State* > states; // States scheduled to be processed
  std::set< FA::State* > epsilons; // End states of epsilon transitions
  std::map< std::string, FAState_s* >::iterator it;

  // Search all states from the starting state
  states.push_back(fa->start);

  while (!states.empty())
  { // Take the first state not visited yet and process it
    current = states.front();
    states.pop_front();

    for (it = current->transitions.begin(); it != current->transitions.end();
      ++it)
    { // Skip epsilon transitions
      if (it->first.empty()) continue;

      // Most distant state accessible using the current transition
      target = it->second;

      try
      { // Try to move to a more distant state using epsilon transitions
        for (;;)
        { // Assume we can advance from this state using epsilon transition
          epsilon = target;
          // Try to actually advance from the state using epsilon transition
          target = target->transitions.at("");
          // We get here only if we advanced successfully so we can redirect
          // the current transition to target state and remove epsilon state
          epsilons.insert(epsilon);
        };
      }
      catch (std::out_of_range& e) {}

      if (target != it->second)
      { // Update the transition to move directly to the target state
        current->transitions[it->first] = target;
      }

      if (visited.count(target) == 0)
      { // Schedule the state where the transition leads for processing
        states.push_back(target);
        // Mark this state as visited as we now know we will process it
        visited.insert(target);
      }
    }
  }

  for (std::set< FA::State* >::iterator eit = epsilons.begin();
    eit != epsilons.end(); ++eit)
  { // Remove all states not used by the FA anymore
    delete (*eit);
  }

  return fa;
}

/** End of file contract.cpp **/
