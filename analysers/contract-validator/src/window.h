/**
 * @brief Contains definition of a class representing a window.
 *
 * A file containing definition of a class representing a window.
 *
 * @file      window.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-02-24
 * @version   0.2
 */

#ifndef __WINDOW_H__
  #define __WINDOW_H__

#include <vector>

#include "pin.H"

#include "contract.h"
#include "vc.hpp"

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
 * @date      Last Update 2016-02-24
 * @version   0.2
 */
class Window
{
  private: // Internal type definitions
    /**
     * @brief A structure representing a single element of the sparse matrix.
     */
    typedef struct Element_s
    {
      struct
      { // Vector clocks for the last and running target/spoiler instances
        VectorClock start; //!< Start of the last target/spoiler instance.
        VectorClock end; //!< End of the last target/spoiler instance.
        VectorClock running; //!< Start of the running target/spoiler instance.
      } vc;
      FARunner* far; //!< Currently running target/spoiler instance.

      /**
       * Constructs a new element of the sparse matrix.
       *
       * @param fa A finite automaton representing a target or spoiler which is
       *   described by this element.
       */
      Element_s(FA* fa) : far(new FARunner(fa)) {};
    } Element;
  private: // Internal data
    THREADID m_tid; //!< A thread owning the window.
     std::vector< Element* > m_targets; //!< Rows of the sparse matrix.
     std::vector< Element* > m_spoilers; //!< Columns of the sparse matrix.
  public: // Constructors
    Window(THREADID tid) : m_tid(tid) {};
  public: // Registration methods
    void monitor(Contract* contract);
  public: // Callback methods changing the information in trace window
    void functionEntered(const std::string& name);
    void functionExited(const std::string& name);
};

#endif /* __WINDOW_H__ */

/** End of file window.h **/
