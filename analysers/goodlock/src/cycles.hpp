/*
 * Copyright (C) 2012-2020 Jan Fiedor <fiedorjan@centrum.cz>
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
 * @brief Contains implementation of functions for enumerating graph cycles.
 *
 * A file containing implementation of functions for enumerating all cycles in
 *   a directed graph (or multigraph).
 *
 * @file      cycles.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-14
 * @date      Last Update 2020-03-06
 * @version   0.2
 */

#ifndef __CYCLES_HPP__
  #define __CYCLES_HPP__

#include <list>
#include <stack>

#include <boost/graph/depth_first_search.hpp>

// A vertex property representing a vertex mark used by the tarjan's algorithm
enum vertex_mark_t { vertex_mark };

namespace boost
{ // All properties must be registered in the boost namespace
  BOOST_INSTALL_PROPERTY(vertex, mark);

#ifdef __CDT_PARSER__
  // CODAN cannot unpack Boost ArgPack parameters correctly, provide a function
  // definition with the required (called) parameters explicitly enumerated
  template < class VertexListGraph, class DFSVisitor, class ColorMap >
  void depth_first_search(const VertexListGraph& g, DFSVisitor vis)
  {
    depth_first_search(g, vis, boost::detail::make_color_map_from_arg_pack(white_color));
  }
#endif
}

/**
 * @brief A structure holding the type of a graph cycle.
 *
 * @tparam Graph A graph (or multigraph).
 */
template< class Graph >
struct Cycle
{
  typedef std::list <
    typename boost::graph_traits< Graph >::edge_descriptor
  > type;
};

/**
 * @brief A structure holding the type of a list of graph cycles.
 *
 * @tparam Graph A graph (or multigraph).
 */
template< class Graph >
struct CycleList
{
  typedef std::list<
    typename Cycle< Graph >::type
  > type;
};

/**
 * @brief A handler for handling cycles found in a graph.
 *
 * Handles cycles found in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2020-03-06
 * @date      Last Update 2020-03-06
 * @version   0.1
 */
template< class Graph >
class CycleHandler
{
  public:
    /**
     * Handles a cycle found in a graph (or multigraph).
     *
     * @param cycle A cycle found in a graph (or multigraph).
     */
    void handleCycle(typename Cycle< Graph >::type& cycle) {};
};

/**
 * @brief A handler for collecting cycles found in a graph.
 *
 * Collects cycles found in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2020-03-06
 * @date      Last Update 2020-03-06
 * @version   0.1
 */
template< class Graph >
class CycleCollector : public CycleHandler< Graph >
{
  private:
    /**
     * @brief A list of cycles found in a graph (or multigraph).
     */
    typename CycleList< Graph >::type& m_cl;
  public:
    /**
     * Constructs a handler for collecting cycles found in a graph.
     *
     * @param cl A list for storing cycles found in the graph.
     */
    CycleCollector(typename CycleList< Graph >::type& cl) : m_cl(cl) {};
  public:
    /**
     * Adds a cycle found in a graph (or multigraph) to the list of cycles.
     *
     * @param cycle A cycle found in a graph (or multigraph).
     */
    void handleCycle(typename Cycle< Graph >::type& cycle)
    {
      m_cl.push_back(cycle);
    }
};

namespace detail
{
  /**
   * @brief An object which creates property maps mapping vertex marks to
   *   boolean values.
   */
  static const
    boost::detail::make_property_map_from_arg_pack_gen< vertex_mark_t, bool>
      make_mark_map_from_arg_pack(false);

  /**
   * @brief A DFS visitor which enumerates all cycles in a graph.
   *
   * Enumerates all cycles in a graph (or multigraph).
   *
   * @tparam Graph A type of the graph (or multigraph).
   * @tparam Handler A class for handling cycles found in the graph.
   * @tparam MarkMap A type of the property map mapping vertices to marks.
   *
   * @author    Jan Fiedor (fiedorjan@centrum.cz)
   * @date      Created 2012-03-14
   * @date      Last Update 2012-03-16
   * @version   0.1
   */
  template< class Graph, class Handler, class MarkMap >
  struct cycles_enumerator : public boost::dfs_visitor<>
  {
    private: // Internal type definitions
      typedef typename boost::graph_traits< Graph >::vertex_descriptor Vertex;
      typedef typename boost::graph_traits< Graph >::edge_descriptor Edge;
    private: // Internal state variables
      const Graph& m_graph;
      Handler& m_handler;
      MarkMap m_marked;
    private: // Internal helper variables
      std::stack< Vertex > m_markedStack;
      std::list< Vertex > m_vertexPath;
      std::list< Edge > m_edgePath;
    public: // Constructors
      /**
       * Constructs a cycles_enumerator object.
       *
       * @param g A graph (or multigraph).
       * @param handler A handler for cycles found in the graph.
       * @param mark A property map mapping vertices to marks.
       */
      cycles_enumerator(const Graph& g, Handler& handler, MarkMap mark)
        : m_graph(g), m_handler(handler), m_marked(mark) {}

    public: // Visitor callback methods
      /**
       * Finds all cycles beginning (and ending) in a visited graph vertex.
       *
       * @note This method is called when a vertex is visited for the first
       *   time.
       *
       * @param u A graph vertex.
       * @param g A graph (or multigraph).
       */
      void discover_vertex(Vertex u, const Graph& g)
      {
        // Find all distinct paths from this vertex to itself (i.e. all cycles)
        this->backtrack(u);

        while (!m_markedStack.empty())
        { // Unmark all marked vertices before processing the next vertex
          put(m_marked, m_markedStack.top(), false);
          m_markedStack.pop();
        }
      }

    private: // Internal methods
      /**
       * Finds all distinct paths from the starting vertex to itself (cycles).
       *
       * @param u The last vertex on the currently searched path.
       * @return @em True if a at least one cycle was found, @em false
       *   otherwise.
       */
      bool backtrack(Vertex u)
      {
        // We did not find a cycle yet
        bool found = false;

        // TODO: might be omitted
        m_vertexPath.push_back(u);

        // Mark the current vertex as a vertex which belongs to the path
        put(m_marked, u, true);

        // Remember that we marked the vertex
        m_markedStack.push(u);

        // Helper methods
        typename boost::graph_traits< Graph >::out_edge_iterator it, end;

        for (boost::tie(it, end) = out_edges(u, m_graph); it != end; it++)
        { // Search all outgoing edges from the current vertex
          Edge e = boost::implicit_cast< Edge >(*it);
          // Get the vertex in which the outgoing edge ends
          Vertex v = target(e, m_graph);

          // Vertex descriptors might not be integers, use their indexes instead
          #define INDEX(vertex) get(boost::vertex_index, m_graph, vertex)

          if (INDEX(v) == INDEX(m_vertexPath.front()))
          { // If the edge is leading to the starting vertex , we found a cycle
            m_edgePath.push_back(e); // TODO: can be moved
            // Let the given handler decide what to do with the cycle found
            m_handler.handleCycle(m_edgePath);
            m_edgePath.pop_back(); // TODO: can be moved
            // Remember that we found a cycle
            found = true;
          }
          else if (INDEX(v) > INDEX(m_vertexPath.front()) && !get(m_marked, v))
          { // The current path might lead to an undiscovered cycle, because:
            // 1) Vertex is not on the currently searched path (if the vertex
            //    was marked, it is already on the path and the cycle will be
            //    detected later when the vertex will be the starting vertex)
            // 2) Vertex has a higher index so the current path might lead to
            //    an undiscovered cycle (if the vertex had a lower index then
            //    even if the path would lead to a cycle, the cycle would be
            //    discovered before when the vertex was the starting vertex)
            m_edgePath.push_back(e); // TODO: can be moved
            found = found | this->backtrack(v);
            m_edgePath.pop_back(); // TODO: can be moved
          }
        }

        if (found)
        { // We normally leave some vertices not on the currently searched path
          // marked when they never lead to the starting vertex and paths from
          // them never close the cycle, but when a cycle is found, we need to
          // unmark them so that they will be searched from other partial paths
          while (m_markedStack.top() != u)
          { // Unmark all vertices marked when searching for paths from here
            put(m_marked, m_markedStack.top(), false);
            m_markedStack.pop();
          }
          // Unmark the current vertex too
          put(m_marked, u, false);
          m_markedStack.pop();
        }

        // TODO: might be omitted
        m_vertexPath.pop_back();

        // Signal that we found at least one cycle
        return found;
      }
  };

  /**
   * Enumerates all cycles in a graph (or multigraph).
   *
   * @tparam Graph A type of the graph (or multigraph).
   * @tparam Handler A class for handling cycles found in the graph.
   * @tparam MarkMap A type of the property map mapping vertices to marks.
   *
   * @param g A graph (or multigraph).
   * @param handler A handler for cycles found in the graph.
   * @param mark A property map mapping vertices to marks.
   */
  template< class Graph, class Handler, class MarkMap >
  void cycles_impl(const Graph& g, Handler& handler, MarkMap mark)
  {
    // Helper variables
    typename boost::graph_traits< Graph >::vertex_iterator it, end;

    // Calling boost::vertices() gives us compilation errors, but without the
    // boost namespace specification, Eclipse CDT does not find the function
    using namespace boost;

    for (boost::tie(it, end) = vertices(g); it != end; it++)
    { // Initialise all marks to their default values (false)
      put(mark, *it, false);
    }

    // We can store the state we need to maintain during the search in a visitor
    cycles_enumerator< Graph, Handler, MarkMap > ce(g, handler, mark);

    // And then use DFS and the visitor to easily iterate through all vertices
    boost::depth_first_search(g, boost::visitor(ce));
  }
}

/**
 * Enumerates all cycles in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 * @tparam Handler A class for handling cycles found in the graph.
 * @tparam P A type of the value of a boost graph library named property.
 * @tparam T A tag identifying the boost graph library named property.
 * @tparam R A base type of the boost graph library named property.
 *
 * @param g A graph (or multigraph).
 * @param handler A handler for cycles found in the graph.
 * @param params A list of boost graph library named properties.
 */
template< class Graph, class Handler, class P, class T, class R >
inline
void cycles(const Graph& g, Handler& handler,
  const boost::bgl_named_params< P, T, R >& params)
{
  // The boost graph library named properties are defined in this namespace
  using namespace boost::graph::keywords;
  // Type definitions
  typedef boost::bgl_named_params< P, T, R > params_type;
  // Process the boost graph library named properties and stored them
  BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)
  // Call the function implementing the enumeration of graph cycles
  detail::cycles_impl(g, handler, detail::make_mark_map_from_arg_pack(g, arg_pack));
}

/**
 * Enumerates all cycles in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 * @tparam Handler A class for handling cycles found in the graph.
 *
 * @param g A graph (or multigraph).
 * @param handler A handler for cycles found in the graph.
 */
template< class Graph, class Handler >
inline
void cycles(const Graph& g, typename boost::enable_if<
  boost::is_base_of< CycleHandler< Graph >, Handler >, Handler >::type& handler)
{
  cycles< Graph, Handler >(g, handler, boost::bgl_named_params< int, int >(0));
}

/**
 * Enumerates all cycles in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 *
 * @param g A graph (or multigraph).
 * @param cl A list containing all cycles found in the graph (or multigraph).
 */
template< class Graph >
inline
void cycles(const Graph& g, typename CycleList< Graph >::type& cl)
{
  // Create a handler which collects all cycles found to the given list
  CycleCollector< Graph > collector(cl);

  // Collect all cycles found in the given graph to the given list
  cycles< Graph, CycleCollector< Graph > >(g, collector);
}

#endif /* __CYCLES_HPP__ */

/** End of file cycles.hpp **/
