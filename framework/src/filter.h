/**
 * @brief Contains definitions used by the tree filter.
 *
 * A file containing definitions of structures, classes, and functions used by
 *   the tree filter.
 *
 * @file      filter.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-06-27
 * @version   0.1
 */

#ifndef __ANACONDA_FRAMEWORK__FILTER_H__
  #define __ANACONDA_FRAMEWORK__FILTER_H__

#include <list>
#include <regex>

/**
 * @brief A hierarchical filter forming a generic tree.
 *
 * A class representing a hierarchical filter forming a generic tree. Each node
 *   of this tree contains a regular expression and any number of child nodes.
 *
 * Given a sequence of strings, it is a match for the tree filter is it finds a
 *   path from the root node to a list node where each of the nodes on the path
 *   matches the corresponding part (string) in the sequence of strings. Please
 *   note that not all parts (strings) of the sequence need to be matched! Only
 *   the parts corresponding to the nodes on the path found are matched and the
 *   remaining parts are assumed to match implicitly, i.e., all (non-existent)
 *   nodes after the list node are considered to match any string.
 *
 * Matching can also be done incrementally, i.e., one can match only the first
 *   part of a sequence. If no match (path) is found, the filter returns a hint
 *   that contains all paths (subtrees) whose first node matches the first part
 *   of the sequence. This hint can then be used to speedup the matching of the
 *   second part of the sequence as the filter can continue its search by using
 *   only the paths that can still match the whole sequence.
 *
 * @note This class acts as a root node of the tree. However, it is not matched
 *   against the sequence of strings. It serves as a starting point from which
 *   the searching begins. In other words, the first part of the sequence is
 *   actually matched against the child nodes of this 'root node'.
 *
 * @tparam Data A custom data available to the user at each node of the tree.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-06-27
 * @version   0.1
 */
template < class Data >
class TreeFilter
{
  public: // Type definitions
    /**
     * @brief A node of the tree filter representing a regular expression.
     */
    struct Node
    {
      std::regex regex; //!< An object representing the regular expression.
      std::list< Node* > childs; //!< A collection of child nodes.
      const Node* parent; //!< A reference to the parent node.
      Data data; //!< A custom data available to the user.
    };

    typedef std::list< Node* > Nodes; //!< A collection of nodes.
  private: // Internal data
    Nodes m_includes; //!< A tree determining if a sequence should be included.
    Nodes m_excludes; //!< A tree determining if a sequence should be excluded.
};

#endif /* __ANACONDA_FRAMEWORK__FILTER_H__ */

/** End of file filter.h **/
