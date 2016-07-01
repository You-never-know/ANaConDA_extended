/**
 * @brief Contains definitions used by the tree filter.
 *
 * A file containing definitions of structures, classes, and functions used by
 *   the tree filter.
 *
 * @file      filter.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-07-01
 * @version   0.4
 */

#ifndef __ANACONDA_FRAMEWORK__FILTER_H__
  #define __ANACONDA_FRAMEWORK__FILTER_H__

#include <functional>
#include <list>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

// Namespace aliases
namespace fs = boost::filesystem;

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
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-07-01
 * @version   0.4
 */
class GenericTreeFilter
{
  public: // Public type definitions
    /**
     * @brief An enumeration of possible error codes that may be returned by
     *   the methods of this class.
     */
    typedef enum Error_e
    {
      NO_ERROR,       //!< No error.
      FILE_NOT_FOUND, //!< File not found.
      INVALID_FILTER  //!< Invalid filter specification.
    } Error;

    /**
     * @brief An enumeration of possible outcomes of matching process.
     */
    typedef enum MatchState_e
    {
      INVALID,        //!< Unexpected outcome.
      MATCH_FOUND,    //!< Found a path (match) satisfying a sequence.
      NO_MATCH_FOUND, //!< There is no path (match) satisfying a sequence.
      /**
       * @note This outcome represent a situation when a path (match) is not
       *   found, however, not all paths were ruled out. In other words, the
       *   sequence given satisfies a prefix of some paths, however, it does
       *   not satisfy any of these paths as a whole. Given additional parts
       *   of the sequence, a path may still be found. This situation mostly
       *   happens when doing incremental matching where the input sequence
       *   is matched part by part and not as a whole sequence at once.
       */
      MATCH_POSSIBLE  //!< Path (match) not found yet, still may be possible.
    } MatchState;

    // Types of functions working with the custom data available at each node
    typedef void* (*GenericDataConstructor)();
    typedef void (*GenericDataDestructor)(void*);
    // Allow to use any function as a processor, even lambdas with captures
    typedef std::function<std::string (const std::string& line, void* data,
      unsigned int level)> GenericDataProcessor;
    typedef std::list< void* > GenericPath;

  protected: // Internal type definitions
    /**
     * @brief A node of the tree filter representing a regular expression.
     */
    typedef struct Node_s
    {
      std::regex regex; //!< An object representing the regular expression.
      std::list< Node_s* > childs; //!< A collection of child nodes.
      Node_s* parent; //!< A reference to the parent node.
      void* data; //!< A custom data available to the user.

      /**
       * Constructs a new node with default values.
       */
      Node_s() : regex(), childs(), parent(NULL), data(NULL) {}
    } Node;

    /**
     * @brief A structure containing all functions needed to work with the
     *   custom data available at each node.
     */
    typedef struct GenericDataHandlers_s
    {
      /**
       * @brief A function for creating (and initialising) the custom data.
       */
      GenericDataConstructor constructor;
      /**
       * @brief A function for disposing (freeing) the custom data.
       */
      GenericDataDestructor destructor;
      /**
       * @brief A function for updating the custom data based on the input.
       */
      GenericDataProcessor processor;

      /**
       * Constructs a new set of handlers. All handlers are set to @c NULL.
       */
      GenericDataHandlers_s() : constructor(NULL), destructor(NULL),
        processor() {}
    } GenericDataHandlers;

    typedef std::list< Node* > Nodes; //!< A collection of nodes.

  public: // Public type definitions encapsulating internal types
    /**
     * @brief A structure containing the result of a matching process.
     *
     * @note The result can be used as a hint when doing incremental matching.
     */
    typedef struct MatchResult_s
    {
      // Allow the tree filter to access the internal data of this structure
      friend class GenericTreeFilter;

      private: // Encapsulated internal data
        /**
         * @brief The current state of the matching process.
         */
        unsigned int state;
        /**
         * @brief A collection of paths that (can still) match a sequence.
         *
         * The content of this collection depends on the state of the matching
         *   process. If a match is found ( state is @c MATCH_FOUND ), it will
         *   contain one or more paths (leaf nodes) matching the sequence that
         *   was matched. If a match is not found, yet still possible, ( state
         *   is @c MATCH_POSSIBLE ), it will contain paths (non-leaf nodes) that
         *   match the currently known parts of the sequence. Until the rest of
         *   the sequence is checked (and a leaf node is found or not), it is
         *   not possible to tell if a match is possible or not. If no match is
         *   found ( state is @c NO_MATCH_FOUND ), the collection will be empty.
         */
        Nodes nodes;

      public: // Public constructors
        /**
         * Construct a match result with @c INVALID state.
         */
        MatchResult_s() : state(INVALID), nodes() {}

      private: // Internal constructors
        /**
         * Construct a match results containing the root node.
         *
         * @param root A root node from which the matching should start.
         */
        MatchResult_s(Node* root) : state(MATCH_POSSIBLE), nodes()
        {
          nodes.push_back(root);
        }

      public: // Methods for reseting the result
        /**
         * Clears the result, i.e., resets it to an @c INVALID state.
         */
        void clear()
        {
          state = INVALID;
          nodes.clear();
        }
    } MatchResult;

  protected: // Internal data
    Node* m_filter; //!< A root node of the tree of regular expressions.
    /**
     * @brief A set of functions for managing the custom data available at each
     *   node.
     */
    GenericDataHandlers m_handlers;
    std::string m_error; //!< A description of the last encountered error.
  public: // Constructors
    /**
     * Constructs a new generic tree filter without any handlers.
     */
    GenericTreeFilter() : m_filter(new Node()), m_handlers(), m_error() {}
  public: // Methods for loading the filter
    int load(fs::path file);
  public: // Methods for matching (parts of) sequences with the filter
    int match(std::string str, MatchResult& result);
    int match(std::string str, MatchResult& result, MatchResult& hint);
  public: // Methods for accessing paths (matches)
    /**
     * Get the custom data at each node of a first path present in a result of
     *   a matching process. The data are ordered from the root node to a leaf
     *   node, excluding the data from the root node as it does not have any
     *   data and is used only as a base for performing matches.
     *
     * @warning Only the data of the first path will be returned.
     *
     * @param result A result of a matching process.
     * @return A collection of custom data stored at each node of a first path
     *   present in a result of a matching process.
     */
    GenericPath getPath(const MatchResult& result)
    {
      // Helper variables
      GenericPath path;

      // Return empty path if the result contains no node
      if (result.nodes.empty()) return path;

      for (Node* node = result.nodes.front(); node != NULL; node = node->parent)
      { // Return a path for the first node, we are going backward in the path
        if (node->data != NULL)
        { // Ignore the root node which does not have any data assigned do it
          path.push_front(node->data);
        }
      }

      return path; // Return the constructed path
    }
};

/**
 * @brief A hierarchical filter forming a generic tree.
 *
 * @tparam Data A custom data available to the user at each node of the tree.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-28
 * @date      Last Update 2016-07-01
 * @version   0.3
 */
template < class Data >
class TreeFilter : public GenericTreeFilter
{
  public: // Public type definitions
    // Allow to use any function as a processor, even lambdas with captures
    typedef std::function<std::string (const std::string& line, Data& data,
      unsigned int level)> DataProcessor;
    typedef std::list< Data* > Path;
  public: // Constructors
    /**
     * Constructs a new hierarchical filter with default custom data handlers.
     */
    TreeFilter()
    {
      // Default data constructor: allocate the custom data on the heap
      m_handlers.constructor = [] () -> void* {
        return static_cast< void* >(new Data());
      };

      // Default data destructor: delete the custom data from the heap
      m_handlers.destructor = [] (void* data) {
        delete static_cast< Data* >(data);
      };

      // Default data processor: return the input as the regular expression
      m_handlers.processor = [] (const std::string& line, void* data,
        unsigned int level) -> std::string {
        return line;
      };
    }

    /**
     * Constructs a new hierarchical filter with custom data processor.
     *
     * @param processor A function processing the input regular expression and
     *   transforming it to a regular expression that will be used by the tree
     *   filter. This function can be used to update the custom data stored at
     *   the node representing the regular expression and also to change the
     *   regular expression itself before it is stored in the node.
     */
    TreeFilter(DataProcessor processor) : TreeFilter()
    {
      // Custom data processor: transform the data and forward the call
      m_handlers.processor = [processor] (const std::string& line, void* data,
        unsigned int level) -> std::string {
        return processor(line, *static_cast< Data* >(data), level);
      };
    }

  public: // Methods for accessing paths (matches)
    /**
     * Get the custom data at each node of a first path present in a result of
     *   a matching process. The data are ordered from the root node to a leaf
     *   node, excluding the data from the root node as it does not have any
     *   data and is used only as a base for performing matches.
     *
     * @warning Only the data of the first path will be returned.
     *
     * @param result A result of a matching process.
     * @return A collection of custom data stored at each node of a first path
     *   present in a result of a matching process.
     */
    Path getPath(const MatchResult& result)
    {
      // Helper variables
      Path path;

      BOOST_FOREACH(void* data, this->GenericTreeFilter::getPath(result))
      { // Transform the generic path to a concrete path with the given data
        path.push_back(static_cast< Data* >(data));
      }

      return path; // Return the transformed path
    }
};

#endif /* __ANACONDA_FRAMEWORK__FILTER_H__ */

/** End of file filter.h **/
