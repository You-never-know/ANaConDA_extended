/**
 * @brief Contains implementation of a class representing a window.
 *
 * A file containing implementation of a class representing a window.
 *
 * @file      window.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-02-28
 * @version   0.4
 */

#include "window.h"

#include <assert.h>

/**
 * Enables monitoring of targets and spoilers from a specific contract.
 *
 * @param contract A contract to which the targets and spoilers to be monitored
 *   belong.
 */
void Window::monitor(Contract* contract)
{
  for (Target* target : contract->getTargets())
  { // Add all targets into the sparse matrix
    if (target->type >= m_targets.size())
    { // Allocate a row for the target if needed
      m_targets.resize(target->type + 1, NULL);
    }

    // There should not be two targets of the same type
    assert(m_targets[target->type] == NULL);

    // Insert the information about the target to the sparse matrix
    m_targets[target->type] = new Element(target->fa);

    for (Spoiler* spoiler : target->spoilers)
    { // Add all spoilers which may violate this target to the sparse matrix
      if (spoiler->type >= m_spoilers.size())
      { // Allocate a column for the spoiler if needed
        m_spoilers.resize(spoiler->type + 1, NULL);
      }

      // There should not be two spoilers of the same type
      assert(m_spoilers[spoiler->type] == NULL);

      // Insert the information about the spoiler to the sparse matrix
      m_spoilers[spoiler->type] = new Element(spoiler->fa);
    }
  }
}

/**
 * Tries to advance all running target and spoiler instances.
 *
 * @param name A name of a function started in the thread owning this window.
 */
void Window::functionEntered(const std::string& name)
{
  for (Element* e : m_targets)
  { // Try to advance all targets
    switch (e->far->advance(name))
    { // Try to advance the current target
      case FARunner::MOVED_TO_NEXT_STATE:
        if (!e->running)
        { // A target instance just started
          e->vc.running = m_cvc;
          e->running = true;
        }
        break;
      case FARunner::NO_TRANSITION_FOUND:
        e->far->reset(); // Search for the next target instance
        e->running = false; // The instance did not start yet
        break;
      case FARunner::INVALID_SYMBOL:
        break;
    }
  }

  for (Element* e : m_spoilers)
  { // Try to advance all spoilers
    switch (e->far->advance(name))
    { // Try to advance the current spoiler
      case FARunner::MOVED_TO_NEXT_STATE:
        if (!e->running)
        { // A spoiler instance just started
          e->vc.running = m_cvc;
          e->running = true;
        }
        break;
      case FARunner::NO_TRANSITION_FOUND:
        e->far->reset(); // Search for the next target instance
        e->running = false; // The instance did not start yet
        break;
      case FARunner::INVALID_SYMBOL:
        break;
    }
  }
}

/**
 * Determines if a target instance was violated by a spoiler instance.
 *
 * @param name A name of a function exited in the thread owning this window.
 */
void Window::functionExited(const std::string& name)
{
  for (Element* e : m_targets)
  { // Check if any of the targets can be violated by a spoiler
    if (!e->far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of target "
      + e->far->regex() + " finished, vc.running= " + e->vc.running
      + ", cvc=" + m_cvc + "\n");

    e->far->reset(); // Search for the next target instance
    e->running = false; // The instance did not start yet
  }

  for (Element* e : m_spoilers)
  { // Check if any of the spoilers can violate a target
    if (!e->far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of spoiler "
      + e->far->regex() + " finished, vc.running= " + e->vc.running
      + ", cvc=" + m_cvc + "\n");

    e->far->reset(); // Search for the next spoiler instance
    e->running = false; // The instance did not start yet
  }
}

/** End of file window.cpp **/
