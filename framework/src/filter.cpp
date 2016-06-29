/**
 * @brief Contains implementation of functions used by the tree filter.
 *
 * A file containing implementation of functions used by the tree filter.
 *
 * @file      filter.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-06-29
 * @version   0.2
 */

#include "filter.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

/**
 * Loads a hierarchical filter from a file.
 *
 * @param file A file containing filter specification.
 * @return @c NO_ERROR if the filter was loaded successfully. @c FILE_NOT_FOUND
 *   if the file containing the filter was not found. @c INVALID_FILTER if the
 *   filter specification contains some error.
 */
int GenericTreeFilter::load(fs::path file)
{
  if (!fs::exists(file)) return FILE_NOT_FOUND;

  // Open the file containing the filter
  fs::fstream f(file);

  // Helper variables
  std::string line;
  unsigned int level = 0;
  unsigned int lineno = 0;
  Node* node = NULL;
  Node* current = m_filter;

  while (std::getline(f, line) && !f.fail())
  { // Line number is used in error description
    ++lineno;

    // Skip all commented and empty lines
    if (line.empty() || line[0] == '#') continue;

    // Ignore leading and trailing spaces
    boost::trim(line);

    // Process the regular expression
    if (line == "{")
    { // Move to the last child node of the current node
      current = current->childs.back();

      ++level; // Increase the current tree level
    }
    else if (line == "}")
    { // Return to the parent node of the current node
      if (current->parent == NULL)
      { // Only root does not have a parent, we cannot encounter } at root
        m_error = "line " + boost::lexical_cast< std::string >(lineno)
          + ": missing '{' for '}'.";

        return INVALID_FILTER;
      }

      current = current->parent; // Return to parent node

      --level; // Decrease the current tree level
    }
    else
    { // Encountered a regular expression, create a new node for it
      node = new Node();
      node->parent = current;
      node->data = m_handlers.constructor();
      // User can change the input regular expression here using the processor
      node->regex = std::regex(m_handlers.processor(line, node->data, level));

      current->childs.push_back(node);
    }
  }

  return NO_ERROR; // Filter loaded successfully
}

/** End of file filter.cpp **/
