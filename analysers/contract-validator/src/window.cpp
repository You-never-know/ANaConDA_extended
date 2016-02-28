/**
 * @brief Contains implementation of a class representing a window.
 *
 * A file containing implementation of a class representing a window.
 *
 * @file      window.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-02-28
 * @version   0.4.1
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

      // Remember that this spoiler may violate the current target
      m_spoilers[spoiler->type]->conflicting.push_back(target->type);
      m_targets[target->type]->conflicting.push_back(spoiler->type);
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
  Element* fe; // Foreign element (element from other thread)

  for (Element* e : m_targets)
  { // Check if any of the targets can be violated by a spoiler
    if (!e->far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of target "
      + e->far->regex() + " finished, vc.running= " + e->vc.running
      + ", cvc=" + m_cvc + "\n");

    for (Window* window : m_windows)
    { // For all initialised (non-NULL) threads other than this one
      if (window == NULL || window == this) continue;

      // Access information about executed spoiler instances
      const ElementList& spoilers = window->getSpoilers();

      for (Spoiler::Type spoiler : e->conflicting)
      { // For each conflicting spoiler
        fe = spoilers[spoiler];

        // Lock only data we will use
        e->readlock();
        fe->readlock();

        if (fe->vc.start.valid())
        { // If start VC is valid, end VC must also be (they are set together)
          if (!e->vc.running.hb(fe->vc.start, window->getTid())
            && !fe->vc.end.hb(m_cvc, m_tid))
          { // start(spoiler) !< start(target) and end(target) !< end(spoiler)
            // spoiler: start=fe->vc.start, end=fe->vc.end
            // target: start=e->vc.running, end=m_cvc
            CONSOLE("Target " + e->far->regex() + " [Thread " + decstr(m_tid)
              + "] violated by spoiler " + fe->far->regex() + " [Thread "
              + decstr(window->getTid()) + "]!\n");
          }
        }

        // All checks done
        fe->unlock();
        e->unlock();
      }
    }

    e->writelock();

    // Forget the previous target instance, replace it with a new one
    e->vc.start = e->vc.running;
    e->vc.end = m_cvc;

    e->far->reset(); // Search for the next target instance
    e->running = false; // The instance did not start yet

    e->unlock();
  }

  for (Element* e : m_spoilers)
  { // Check if any of the spoilers can violate a target
    if (!e->far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of spoiler "
      + e->far->regex() + " finished, vc.running= " + e->vc.running
      + ", cvc=" + m_cvc + "\n");

    for (Window* window : m_windows)
    { // For all initialised (non-NULL) threads other than this one
      if (window == NULL || window == this) continue;

      // Access information about executed target instances
      const ElementList& targets = window->getTargets();

      for (Target::Type target : e->conflicting)
      { // For each conflicting spoiler
        fe = targets[target];

        // Lock only data we will use
        e->readlock();
        fe->readlock();

        if (fe->vc.start.valid())
        { // If start VC is valid, end VC must also be (they are set together)
          if (!fe->vc.start.hb(e->vc.running, m_tid)
            && !m_cvc.hb(fe->vc.end, window->getTid()))
          { // start(spoiler) !< start(target) and end(target) !< end(spoiler)
            // spoiler: start=e->vc.running, end=m_cvc
            // target: start=fe->vc.start, end=fe->vc.end
            CONSOLE("Target " + e->far->regex() + " [Thread " + decstr(m_tid)
              + "] violated by spoiler " + fe->far->regex() + " [Thread "
              + decstr(window->getTid()) + "]!\n");
          }
        }

        // All checks done
        fe->unlock();
        e->unlock();
      }
    }

    e->writelock();

    // Forget the previous spoiler instance, replace it with a new one
    e->vc.start = e->vc.running;
    e->vc.end = m_cvc;

    e->far->reset(); // Search for the next spoiler instance
    e->running = false; // The instance did not start yet

    e->unlock();
  }
}

/** End of file window.cpp **/
