/**
 * @brief Contains implementation of functions used by the tree filter.
 *
 * A file containing implementation of functions used by the tree filter.
 *
 * @file      filter.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-07-01
 * @version   0.5
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
      if (current->childs.empty())
      { // Cannot have a child node without a parent node
        m_error = "line " + boost::lexical_cast< std::string >(lineno)
          + ": missing parent filter.";

        return INVALID_FILTER;
      }

      current = current->childs.back(); // Move to last child node

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

/**
 * Checks which of a given paths can be satisfied by a part of sequence given.
 *
 * @param str A part of a sequence to be matched against the given paths.
 * @param result A result of the matching process, i.e., a collection of paths
 *   that either satisfy the sequence to which the given path belongs, or may
 *   still satisfy the sequence (which cannot be determined until the rest of
 *   the sequence is matched against these paths).
 * @return @c MATCH_FOUND if a path (match) is found ( paths found can be found
 *   in the @c result ). @c NO_MATCH_FOUND if there is no path that can satisfy
 *   the sequence. @c MATCH_POSSIBLE if no path was found, but not all paths
 *   were ruled out (paths whose prefixes satisfy the sequence can be found in
 *   @c result and one can use these paths to continue the search after adding
 *   additional parts to the sequence to be matched).
 */
int GenericTreeFilter::match(std::string str, MatchResult& result)
{
  // No hint given, start the matching process from the root node
  MatchResult hint(m_filter);

  // Try to find a match for the (part of) sequence given
  return this->match(str, result, hint);
}

/**
 * Checks which of a given paths can be satisfied by a part of sequence given.
 *
 * @param str A part of a sequence to be matched against the given paths.
 * @param result A result of the matching process, i.e., a collection of paths
 *   that either satisfy the sequence to which the given path belongs, or may
 *   still satisfy the sequence (which cannot be determined until the rest of
 *   the sequence is matched against these paths).
 * @param hint A result of the previous matching process. Usually this is the
 *   result of matching the previous part of the sequence against the filter.
 * @return @c MATCH_FOUND if a path (match) is found ( paths found can be found
 *   in the @c result ). @c NO_MATCH_FOUND if there is no path that can satisfy
 *   the sequence. @c MATCH_POSSIBLE if no path was found, but not all paths
 *   were ruled out (paths whose prefixes satisfy the sequence can be found in
 *   @c result and one can use these paths to continue the search after adding
 *   additional parts to the sequence to be matched). @c INVALID if the hint
 *   given is not a result of some previous search.
 */
int GenericTreeFilter::match(std::string str, MatchResult& result,
  MatchResult& hint)
{
  // Need a valid result of some previous match as a hint
  if (hint.state == INVALID) return INVALID;

  // Make sure the result does not contain any data from previous matches
  result.clear();

  // No path (match) found yet
  result.state = NO_MATCH_FOUND;

  BOOST_FOREACH(Node* parent, hint.nodes)
  { // All nodes here are already satisfied, we need to check their children
    BOOST_FOREACH(Node* child, parent->childs)
    { // Find all nodes that match the (part of) sequence
      if (regex_match(str, child->regex))
      { // This node matches the string
        if (child->childs.empty())
        { // Leaf node -> path (match) found
          result.state = MATCH_FOUND;

          // Keep only the path satisfying the sequence
          result.nodes.clear();
          result.nodes.push_back(child);

          return MATCH_FOUND;
        }
        else
        { // Non-leaf node -> path (match) may still be found
          result.state = MATCH_POSSIBLE;

          // Keep all paths that may still be satisfied
          result.nodes.push_back(child);
        }
      }
    }
  }

  return result.state; // Either MATCH_POSSIBLE or NO_MATCH_FOUND
}

/** End of file filter.cpp **/
