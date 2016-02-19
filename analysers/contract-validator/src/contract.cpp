/**
 * @brief Contains implementation of a class representing a contract.
 *
 * A file containing implementation of a class representing a contract.
 *
 * @file      contract.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-18
 * @date      Last Update 2016-02-19
 * @version   0.1
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
    typedef boost::tokenizer< boost::char_separator< char > > Tokenizer;
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
  std::cout << regex << "\n";

  return NULL;
}

/** End of file contract.cpp **/
