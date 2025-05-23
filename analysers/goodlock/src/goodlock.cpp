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
 * @brief Contains the entry part of the GoodLock ANaConDA plugin.
 *
 * A file containing the entry part of the GoodLock ANaConDA plugin.
 *
 * @file      goodlock.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-03-09
 * @date      Last Update 2020-03-07
 * @version   0.5
 */

#include <map>
#include <set>

#include <boost/graph/adjacency_list.hpp>

#include "anaconda/anaconda.h"

#include "anaconda/utils/plugin/settings.hpp"

#include "cycles.hpp"

#ifdef BOOST_NO_EXCEPTIONS
// Exceptions cannot be used so we must define the throw_exception() manually
namespace boost { void throw_exception(std::exception const& e) { return; } }
#endif

// An edge property containing information about a lock graph edge
enum edge_info_t { edge_info };

namespace boost
{ // All properties must be registered in the boost namespace
  BOOST_INSTALL_PROPERTY(edge, info);
}

// Type definitions
typedef std::set< int > LockSet;
typedef std::map< LOCK, unsigned int > LockMap;

/**
 * @brief A structure containing additional information about a lock graph edge.
 *
 * Contains additional information about a lock graph edge needed to get rid of
 *   the cycles which do not cause deadlocks (i.e. lead to false alarms).
 */
typedef struct EdgeInfo_s
{
  THREADID thread; //!< A number identifying a thread which acquired a lock.
  LockSet lockset; //!< A set of locks held by a thread which acquired a lock.

  /**
   * Constructs an EdgeInfo_s object.
   */
  EdgeInfo_s() : thread(0), lockset() {}

  /**
   * Constructs an EdgeInfo_s object.
   *
   * @param t A number identifying a thread which acquired a lock.
   * @param l A set of locks held by a thread which acquired a lock.
   */
  EdgeInfo_s(THREADID t, LockSet& l) : thread(t), lockset(l) {}
} EdgeInfo;

// Type definitions
typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS,
  boost::no_property, boost::property< edge_info_t, EdgeInfo > > LockGraph;

// Declarations of static functions (usable only within this module)
static VOID deleteLockSet(void* lset);

namespace
{ // Static global variables (usable only within this module)
  TLS_KEY g_lockSetTlsKey = TLS_CreateThreadDataKey(deleteLockSet);

  LockMap g_lockMap; //!< A table mapping lock object to vertexes.
  LockGraph g_lockGraph; //!< A lock graph.

  Settings g_settings; //!< An object holding the plugin's settings.
}

/**
 * Concatenates a string with additional information about a lock graph edge.
 *
 * @param s A string.
 * @param info An object containing additional information about a lock graph
 *   edge.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em info.
 */
std::string operator+(const std::string& s, const EdgeInfo& info)
{
  // Helper variable
  LockSet::iterator it;

  // Append information about a thread which acquired a lock
  std::string sinfo(decstr(info.thread) + ",{");

  for (it = info.lockset.begin(); it != info.lockset.end(); it++)
  { // Append information about locks held by the thread when acquiring a lock
    sinfo += ((it == info.lockset.begin()) ? decstr(*it) : "," + decstr(*it));
  }

  // Return the new string
  return s  + sinfo + "}";
}

/**
 * Concatenates a string with a lock graph edge.
 *
 * @param s A string.
 * @param e A lock graph edge.
 * @return A new string with a value of @em s followed by a string
 *   representation of @em e.
 */
std::string operator+(const std::string& s,
  const boost::graph_traits< LockGraph >::edge_descriptor& e)
{
  return s  + "(" + decstr(source(e, g_lockGraph)) + ",(" + get(edge_info,
    g_lockGraph, e) + ")," + decstr(target(e, g_lockGraph)) + ")";
}

/**
 * Deletes a lock set created during thread start.
 *
 * @param lset A lock set.
 */
VOID deleteLockSet(void* lset)
{
  delete static_cast< LockSet* >(lset);
}

/**
 * Gets a lock set associated with a specific thread.
 *
 * @param tid A number uniquely identifying the thread.
 * @return The lock set associated with the specified thread.
 */
inline
LockSet* getLockSet(THREADID tid)
{
  return static_cast< LockSet* >(TLS_GetThreadData(g_lockSetTlsKey, tid));
}

/**
 * Prints all edges of a lock graph.
 */
void printLockGraph()
{
  // Helper variables
  boost::graph_traits< LockGraph >::vertex_iterator vit, vend;
  boost::graph_traits< LockGraph >::out_edge_iterator eit, eend;

  // Print the basic information about a lock graph
  CONSOLE_NOPREFIX("Lock Graph\n----------\n");
  CONSOLE_NOPREFIX("Vertices: " + decstr(num_vertices(g_lockGraph)) +  "\n");
  CONSOLE_NOPREFIX("Edges: " + decstr(num_edges(g_lockGraph)) +  "\n");

  for (boost::tie(vit, vend) = vertices(g_lockGraph); vit != vend; vit++)
  { // Process all outgoing edges of each vertex of the lock graph
    for (boost::tie(eit, eend) = out_edges(*vit, g_lockGraph); eit != eend;
      eit++)
    { // Print information about a lock graph edge
      CONSOLE_NOPREFIX("  Edge " + *eit + "\n");
    }
  }

  CONSOLE_NOPREFIX("\n");
}

/**
 * Prints a potential deadlock.
 *
 * @param cycle A cycle that may be a potential deadlock.
 */
void printPotentialDeadlock(Cycle< LockGraph >::type& cycle)
{
  // Helper variables
  Cycle< LockGraph >::type::iterator cit;
  std::set< THREADID > tset;
  LockSet::iterator it;
  LockSet lset;

  // Text describing a potential deadlock (valid cycle in a lock graph)
  std::string cstring("Cycle ");

  // Presume that the cycle is valid
  bool valid = true;

  for (cit = cycle.begin(); cit != cycle.end(); cit++)
  { // Check if the cycle is not single-threaded or guarded cycle
    EdgeInfo& info = get(edge_info, g_lockGraph, *cit);

    // Each lock in the cycle must be obtained by a different thread
    if (!(valid = tset.insert(info.thread).second)) break;

    for (it = info.lockset.begin(); it != info.lockset.end(); it++)
    { // No two locks can be obtained when holding the same (guard) lock
      if (!(valid = lset.insert(*it).second)) break;
    }

    // Valid so far, append information about the cycle edge
    cstring += ((cit == cycle.begin()) ? "" : ",") + *cit;
  }

  // Print the information about a lock graph cycle if valid
  if (valid) CONSOLE_NOPREFIX(cstring + "\n");
}

/**
 * @brief A handler for printing potential deadlocks found in a graph.
 *
 * Prints potential deadlocks (valid cycles) found in a graph (or multigraph).
 *
 * @tparam Graph A type of the graph (or multigraph).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2020-03-07
 * @date      Last Update 2020-03-07
 * @version   0.1
 */
template< class Graph >
class CyclePrinter : public CycleHandler< Graph >
{
  public:
    /**
     * Prints potential deadlock found in a graph (or multigraph).
     *
     * @param cycle A cycle found in a graph (or multigraph).
     */
    void handleCycle(typename Cycle< Graph >::type& cycle)
    {
      printPotentialDeadlock(cycle);
    }
};

/**
 * Prints potential deadlocks.
 */
void printPotentialDeadlocks()
{
  // Print all cycles which may cause deadlocks
  CONSOLE_NOPREFIX("Potential Deadlocks\n-------------------\n");

  if (g_settings.get< std::string >("show.deadlocks") == "immediately")
  { // Print potential deadlocks immediately when they are found
    CyclePrinter< LockGraph > printer;

    // Print all potential deadlocks in the lock graph
    cycles< LockGraph, CyclePrinter< LockGraph > >(g_lockGraph, printer);
  }
  else if (g_settings.get< std::string >("show.deadlocks") == "finally")
  { // Print potential deadlocks after enumerating all cycles
    CycleList< LockGraph >::type::iterator clit;
    CycleList< LockGraph >::type cl;

    // Get all cycles present in a lock graph
    cycles< LockGraph >(g_lockGraph, cl);

    for (clit = cl.begin(); clit != cl.end(); clit++)
    { // Print all potential deadlocks among the found cycles
      printPotentialDeadlock(*clit);
    }
  }

  CONSOLE_NOPREFIX("\n");
}

/**
 * Prints information about a lock release.
 *
 * @param tid A thread in which was the lock released.
 * @param lock An object representing the lock released.
 */
VOID beforeLockRelease(THREADID tid, LOCK lock)
{
  // Remove the released lock from the lock set
  getLockSet(tid)->erase(g_lockMap.find(lock)->second);
}

/**
 * Prints information about a lock acquisition.
 *
 * @param tid A thread in which was the lock acquired.
 * @param lock An object representing the lock acquired.
 */
VOID afterLockAcquire(THREADID tid, LOCK lock)
{
  // Get a vertex representing the lock in the lock graph
  LockMap::iterator it = g_lockMap.find(lock);

  if (it == g_lockMap.end())
  { // If no vertex represent the lock in the lock graph, add a new one to it
    it = g_lockMap.insert(LockMap::value_type(lock, add_vertex(g_lockGraph))).first;
  }

  // Helper variables
  boost::graph_traits< LockGraph >::out_edge_iterator eit, eend;

  for (LockSet::iterator lsit = getLockSet(tid)->begin();
    lsit != getLockSet(tid)->end(); lsit++)
  { // Add edges from all held locks to the acquired lock to the lock graph
    bool found = false;

    for (boost::tie(eit, eend) = out_edges(*lsit, g_lockGraph); eit != eend;
      eit++)
    { // Check if the edge to be added is not already present in the lock graph
      if (target(*eit, g_lockGraph) != it->second)
        continue; // This edge leads to a vertex representing different lock

      if (get(edge_info, g_lockGraph, *eit).thread != tid)
        continue; // This edge leads to a lock acquired by different thread

      if (get(edge_info, g_lockGraph, *eit).lockset == *getLockSet(tid))
      { // Edge already present in the lock graph, no need to search further
        found = true;
        break;
      }
    }

    if (!found)
    { // Edge not found in the lock graph, add it
      add_edge(*lsit, it->second, EdgeInfo(tid, *getLockSet(tid)), g_lockGraph);
    }
  }

  // Add the acquired lock to the lock set
  getLockSet(tid)->insert(it->second);
}

/**
 * Prints information about a thread which is about to start.
 *
 * @param tid A number identifying the thread.
 */
VOID threadStarted(THREADID tid)
{
  // Initialise a lock set of the newly created thread
  TLS_SetThreadData(g_lockSetTlsKey, new LockSet(), tid);
}

/**
 * Initialises the GoodLock plugin.
 */
PLUGIN_INIT_FUNCTION()
{
  // Register all settings supported by the analyser
  g_settings.addOptions()
    FLAG("show.lockgraph", false)
    OPTION("show.deadlocks", std::string, "immediately")
    ;

  // Load plugin's settings, continue on error
  LOAD_SETTINGS(g_settings, "goodlock.conf");

  // Register callback functions called before synchronisation events
  SYNC_BeforeLockRelease(beforeLockRelease);

  // Register callback functions called after synchronisation events
  SYNC_AfterLockAcquire(afterLockAcquire);

  // Register callback functions called when a thread starts or finishes
  THREAD_ThreadStarted(threadStarted);
}

/**
 * Finalises the GoodLock plugin.
 */
PLUGIN_FINISH_FUNCTION()
{
  if (g_settings.enabled("show.lockgraph"))
  { // Print all edges in the lock graph
    printLockGraph();
  }

  if (g_settings.get< std::string >("show.deadlocks") != "never")
  { // Print all cycles in the lock graph
    printPotentialDeadlocks();
  }
}

/** End of file goodlock.cpp **/
