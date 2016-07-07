/**
 * @brief Contains definitions used by the tree filter.
 *
 * A file containing definitions of structures, classes, and functions used by
 *   the tree filter.
 *
 * @file      filter.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-06-23
 * @date      Last Update 2016-07-07
 * @version   0.6.3
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
 * @date      Last Update 2016-07-04
 * @version   0.5
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
         * @brief A collection of paths that (can still) match a sequence.
         *
         * The content of this collection depends on the result of the matching
         *   process. If a match is found ( match returned @em true ), it will
         *   contain one or more paths (leaf nodes) matching the sequence that
         *   was matched. If a match is not found, yet still possible, ( match
         *   returned @em false ), it will contain paths (non-leaf nodes) that
         *   match the currently known parts of the sequence. Until the rest of
         *   the sequence is checked (and a leaf node is found or not), it is
         *   not possible to tell if a match is possible or not. If no match is
         *   found ( match returned @em false ), the collection will be empty.
         */
        Nodes nodes;

      public: // Public constructors
        /**
         * Construct an empty match result.
         */
        MatchResult_s() : nodes() {}

      private: // Internal constructors
        /**
         * Construct a match results containing the root node.
         *
         * @param root A root node from which the matching should start.
         */
        MatchResult_s(Node* root) : nodes(1, root) {}

      public: // Methods for clearing the result
        /**
         * Clears a match result.
         */
        void clear()
        {
          nodes.clear();
        }

      public: // Methods for querying the result
        /**
         * Checks if a match result is empty.
         *
         * @return @em True if the match result is empty, @em false otherwise.
         */
        bool empty()
        {
          return nodes.empty();
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
    bool match(std::string str, MatchResult& result);
    bool match(std::string str, MatchResult& result, MatchResult& hint);
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
 * @date      Last Update 2016-07-07
 * @version   0.4
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
      this->setDataProcessor(processor);
    }

  public: // Methods for setting handlers
    /**
     * Sets a new custom data processor.
     *
     * @param processor A function processing the input regular expression and
     *   transforming it to a regular expression that will be used by the tree
     *   filter. This function can be used to update the custom data stored at
     *   the node representing the regular expression and also to change the
     *   regular expression itself before it is stored in the node.
     */
    void setDataProcessor(DataProcessor processor)
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

/**
 * @brief A hierarchical filter using two mutually exclusive tree filters.
 *
 * This filter uses two mutually exclusive tree filters to check if a sequence
 *   matches the filter or not. A sequence matches the filter if and only if a
 *   path (match) is found using the first tree filter and not found using the
 *   second one. So the result of the first (main) filter might be invalidated
 *   by the second filter if he also finds a match.
 *
 * This filter may be useful for implementing various kinds of include/exclude
 *   filters where one want to include something only if it is not excluded at
 *   the same time or the vice versa.
 *
 * @tparam Data A custom data available to the user at each node of the tree.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-07-01
 * @date      Last Update 2016-07-07
 * @version   0.4
 */
template < class Data >
class InvalidatingTreeFilter
{
  public: // Public type definitions
    /**
     * @brief A structure containing the result of a matching process.
     *
     * @note The result can be used as a hint when doing incremental matching.
     */
    typedef struct MatchResult_s
    {
      // Allow the tree filter to access the internal data of this structure
      friend InvalidatingTreeFilter;

      private: // Encapsulated internal data
        /**
         * @brief The result of the first filter (the result may be invalidated
         *   by the second filter).
         */
        typename TreeFilter< Data >::MatchResult main;
        /**
         * @brief The result of the second filter (may invalidate the result of
         *   the first filter).
         */
        typename TreeFilter< Data >::MatchResult invalidating;

      public: // Methods for clearing the result
        /**
         * Clears a match result.
         */
        void clear()
        {
          main.clear();
          invalidating.clear();
        }

      public: // Methods for querying the result
        /**
         * Checks if a match result is empty.
         *
         * @return @em True if the match result is empty, @em false otherwise.
         */
        bool empty()
        {
          return main.empty();
        }
    } MatchResult;

  private: // Internal type definitions
    typedef TreeFilter< Data > Filter;
  private: // Internal data
    /**
     * @brief The first filter whose match may be invalidated by the second
     *   filter.
     */
    Filter m_main;
    /**
     * @brief The second filter that may invalidate the matches found by the
     *   first filter.
     */
    Filter m_invalidating;

  public: // Constructors
    /**
     * Constructs a new invalidating filter with default custom data handlers.
     */
    InvalidatingTreeFilter() : m_main(), m_invalidating() {}

    /**
     * Constructs a new invalidating filter with custom data processor.
     *
     * @param processor A function processing the input regular expression and
     *   transforming it to a regular expression that will be used by the tree
     *   filter. This function can be used to update the custom data stored at
     *   the node representing the regular expression and also to change the
     *   regular expression itself before it is stored in the node.
     */
    InvalidatingTreeFilter(typename Filter::DataProcessor processor)
      : m_main(processor), m_invalidating(processor) {}

  public: // Methods for setting handlers
    /**
     * Sets a new custom data processor for both filters.
     *
     * @param processor A function processing the input regular expression and
     *   transforming it to a regular expression that will be used by the tree
     *   filter. This function can be used to update the custom data stored at
     *   the node representing the regular expression and also to change the
     *   regular expression itself before it is stored in the node.
     */
    void setDataProcessor(typename Filter::DataProcessor processor)
    {
      m_main.setDataProcessor(processor);
      m_invalidating.setDataProcessor(processor);
    }

  public: // Methods for loading the filter
    /**
     * Loads both filters from a file.
     *
     * @param main A file containing the specification of the first filter.
     * @param invalidating A file containing the specification of the second
     *   filter.
     * @return @c NO_ERROR if both filters were loaded successfully.
     *   @c FILE_NOT_FOUND if any of the files containing the filters was not
     *   found. @c INVALID_FILTER if the filter specification contains some
     *   error.
     */
    int load(fs::path main, fs::path invalidating)
    {
      // Helper variables
      int result = 0;

      // Load the first filter
      result = m_main.load(main);

      // Do not continue if the first filter cough not be loaded successfully
      if (result != GenericTreeFilter::NO_ERROR) return result;

      // First filter loaded successfully, load the second filter
      return m_invalidating.load(invalidating);
    }

  public: // Methods for matching (parts of) sequences with the filter
    /**
     * Checks if the first part of a string sequence matches any path present
     *   in the tree filter.
     *
     * Checks if the first string of a sequence of strings matches any of the
     *   regular expressions stored in the nodes under the root node.
     *
     * @param str The first part of a string sequence to be matched against the
     *   regular expressions stored in the nodes under the root node.
     * @param result A result of the matching process. It contains a collection
     *   of paths whose first parts (nodes) matches the first part of the given
     *   sequence.
     * @return @em True if a match is found, @em false otherwise. If a match is
     *   found, the paths that are satisfied by the given sequence can be found
     *   the the @c result of the matching process. If a match is not found, it
     *   may be either because no match is possible (@em result will be empty)
     *   or the sequence given is not long enough to satisfy a whole path. In
     *   the second case, there may be paths that may still be satisfied if the
     *   given sequence is extended with the correct strings and in this case,
     *   the @c result will contain all such paths.
     */
    bool match(std::string str, MatchResult& result)
    {
      // Check for matches using both filters
      bool match = m_main.match(str, result.main);
      m_invalidating.match(str, result.invalidating);

      // Match is found if the 1st filter finds a match and the 2nd one cannot
      // find any (its result must be empty so it is certain that no match can
      // be found, even when continuing with incremental matching)
      return match && result.invalidating.empty();
    }

    /**
     * Checks if a part of a string sequence matches any of the remaining paths
     *   present in the result of the previous matching process.
     *
     * Checks if a string of a sequence of strings matches any of the regular
     *   expressions stored in the nodes under the ones present in the result
     *   of the previous matching process.
     *
     * @param str A part of a string sequence to be matched against the regular
     *   expressions stored in the nodes under the ones present in the result
     *   of the previous matching process.
     * @param result A result of the matching process. It contains a collection
     *   of paths whose parts (nodes) matches the given part of the sequence.
     * @param hint A result of a previous matching process. Usually this is the
     *   result of matching the previous part of the sequence.
     * @return @em True if a match is found, @em false otherwise. If a match is
     *   found, the paths that are satisfied by the given sequence can be found
     *   the the @c result of the matching process. If a match is not found, it
     *   may be either because no match is possible (@em result will be empty)
     *   or the sequence given is not long enough to satisfy a whole path. In
     *   the second case, there may be paths that may still be satisfied if the
     *   given sequence is extended with the correct strings and in this case,
     *   the @c result will contain all such paths.
     */
    bool match(std::string str, MatchResult& result, MatchResult& hint)
    {
      // Check for matches using both filters
      bool match = m_main.match(str, result.main, hint.main);
      m_invalidating.match(str, result.invalidating, hint.invalidating);

      // Match is found if the 1st filter finds a match and the 2nd one cannot
      // find any (its result must be empty so it is certain that no match can
      // be found, even when continuing with incremental matching)
      return match && result.invalidating.empty();
    }
};

#endif /* __ANACONDA_FRAMEWORK__FILTER_H__ */

/** End of file filter.h **/
