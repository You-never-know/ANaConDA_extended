/**
 * @brief Contains definition of a class representing a window.
 *
 * A file containing definition of a class representing a window.
 *
 * @file      window.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-03-01
 * @version   0.7
 */

#ifndef __WINDOW_H__
  #define __WINDOW_H__

#include <vector>

#include "pin.H"

#include "utils/lockobj.hpp"

#include "contract.h"
#include "vc.hpp"

// Forward declarations
class Window;

// Type definitions
typedef std::vector< Window* > WindowList;

/**
 * @brief A class representing a window.
 *
 * Represents a window which contains information about some of the events
 *   encountered in the execution of a program.
 *
 * The window is implemented as a sparse matrix of targets (rows) and spoilers
 *   (columns) where each spoiler can violate only one target, but one target
 *   can be violated by more than one spoiler. As the same spoiler may violate
 *   more than one target in practice, there may be duplicate spoilers in the
 *   matrix (we are tolerating a small redundancy here in exchange for a great
 *   simplification of some parts of the detection algorithm).
 *
 * Note that each element of the sparse matrix is in fact only one half of the
 *   data stored in a concrete cell of the sparse matrix. This is because the
 *   same data needs to be stored for both the target and the spoiler. Also we
 *   often need to access the two separate parts of the data exclusively, and
 *   it is much easier to do so when keeping them separate.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-03-01
 * @version   0.7
 */
class Window
{
  private: // Internal type definitions
    /**
     * @brief A structure containing information about instances of a specific
     *   target or spoiler.
     *
     * @note This structure represents half of a single element of the sparse
     *   matrix. One half contains information about the target instances and
     *   the other half about spoiler instances that may violate the target
     *   instances. They are linked through the @c conflicting attribute.
     */
    typedef struct Instances_s : public RwLockableObject
    {
      struct
      { // Information about the last instance encountered in the execution
        VectorClock start; //!< Time when the instance started its execution.
        VectorClock end; //!< Time then the instance ended its execution.
      } last; //!< Contains information about the last encountered instance.
      struct
      { // Information about the currently running instance
        VectorClock start; //!< Time when the instance started its execution.
        FARunner* far; //!< A finite automaton validating the running instance.
        bool started; //!< A flag used to determine if the instance is running.
      } running; //!< Contains information about the currently running instance.
      /**
       * @brief A list of types of targets or spoilers whose instances may be
       *   violated by the instances of this target or spoiler.
       *
       * @note For instances of spoilers, the list contains only one entry as
       *   only one target can be violated by a spoiler. See the description
       *   of @c Window for more information how everything is implemented.
       */
      std::vector< Spoiler::Type > conflicting;

      /**
       * Constructs a structure containing information about the instances of a
       *   specific target or spoiler.
       *
       * @param fa A finite automaton used to validate instances of a specific
       *   target or spoiler.
       */
      Instances_s(FA* fa)
      {
        running.far = new FARunner(fa);
        running.started = false;
      };
    } Instances;
    typedef std::vector< Instances* > InstancesList;
  private: // Internal data
    THREADID m_tid; //!< A thread owning the window.
    VectorClock& m_cvc; //!< Current vector clock of the thread.
    WindowList& m_windows; //!< A list of windows owned by threads.
    InstancesList m_targets; //!< Rows of the sparse matrix.
    InstancesList m_spoilers; //!< Columns of the sparse matrix.
  public: // Constructors
    /**
     * Constructs a new trace window owned by a specific thread.
     *
     * @param tid A number identifying the thread owning the window.
     * @param cvc The current vector clock of the thread owning the window.
     * @param w A list of trace windows owned by any thread.
     */
    Window(THREADID tid, VectorClock& cvc, WindowList& w) : m_tid(tid),
      m_cvc(cvc), m_windows(w) {};
  public: // Methods for accessing internal data
    /**
     * Gets a number identifying the thread.
     *
     * @return A number identifying the thread.
     */
    const THREADID getTid() { return m_tid; }
    /**
     * Gets information about all target instances encountered in the execution
     *   of the thread owning the window.
     *
     * @return A list containing information about the instances of each target
     *   encountered in the execution of the thread owning the window.
     */
    const InstancesList& getTargets() { return m_targets; }
    /**
     * Gets information about all spoiler instances encountered in the execution
     *   of the thread owning the window.
     *
     * @return A list containing information about the instances of each spoiler
     *   encountered in the execution of the thread owning the window.
     */
    const InstancesList& getSpoilers() { return m_spoilers; }
  public: // Registration methods
    void monitor(Contract* contract);
  public: // Callback methods changing the information in trace window
    void functionEntered(const std::string& name);
    void functionExited(const std::string& name);
  private: // Internal functions used by the detection algorithm
    void advance(Instances* instance, const std::string& name);
    void replaceLast(Instances* instance);
};

#endif /* __WINDOW_H__ */

/** End of file window.h **/
