/**
 * @brief Contains implementation of a class representing a window.
 *
 * A file containing implementation of a class representing a window.
 *
 * @file      window.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-03-01
 * @version   0.5
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
    m_targets[target->type] = new Instances(target->fa);

    for (Spoiler* spoiler : target->spoilers)
    { // Add all spoilers which may violate this target to the sparse matrix
      if (spoiler->type >= m_spoilers.size())
      { // Allocate a column for the spoiler if needed
        m_spoilers.resize(spoiler->type + 1, NULL);
      }

      // There should not be two spoilers of the same type
      assert(m_spoilers[spoiler->type] == NULL);

      // Insert the information about the spoiler to the sparse matrix
      m_spoilers[spoiler->type] = new Instances(spoiler->fa);

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
  for (Instances* instances : m_targets)
  { // Try to advance all targets
    switch (instances->running.far->advance(name))
    { // Try to advance the current target
      case FARunner::MOVED_TO_NEXT_STATE:
        if (!instances->running.started)
        { // A target instance just started
          instances->running.start = m_cvc;
          instances->running.started = true;
        }
        break;
      case FARunner::NO_TRANSITION_FOUND:
        instances->running.far->reset(); // Search for the next target instance
        instances->running.started = false; // The instance did not start yet
        break;
      case FARunner::INVALID_SYMBOL:
        break;
    }
  }

  for (Instances* instances : m_spoilers)
  { // Try to advance all spoilers
    switch (instances->running.far->advance(name))
    { // Try to advance the current spoiler
      case FARunner::MOVED_TO_NEXT_STATE:
        if (!instances->running.started)
        { // A spoiler instance just started
          instances->running.start = m_cvc;
          instances->running.started = true;
        }
        break;
      case FARunner::NO_TRANSITION_FOUND:
        instances->running.far->reset(); // Search for the next spoiler instance
        instances->running.started = false; // The instance did not start yet
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
  // Helper variables
  Instances* instance;

  for (Instances* target : m_targets)
  { // Check if any of the targets can be violated by a spoiler
    if (!target->running.far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of target "
      + target->running.far->regex() + " finished, vc.running= "
      + target->running.start + ", cvc=" + m_cvc + "\n");

    for (Window* window : m_windows)
    { // For all initialised (non-NULL) threads other than this one
      if (window == NULL || window == this) continue;

      // Access information about executed spoiler instances
      const InstancesList& spoilers = window->getSpoilers();

      for (Spoiler::Type spoiler : target->conflicting)
      { // For each conflicting spoiler
        instance = spoilers[spoiler];

        // Lock only data we will use
        target->readlock();
        instance->readlock();

        if (instance->last.start.valid())
        { // If start VC is valid, end VC must also be (they are set together)
          if (!target->running.start.hb(instance->last.start, window->getTid())
            && !instance->last.end.hb(m_cvc, m_tid))
          { // start(spoiler) !< start(target) and end(target) !< end(spoiler)
            // spoiler: start=instance->last.start, end=instance->last.end
            // target: start=target->running.start, end=m_cvc
            CONSOLE("Target " + target->running.far->regex() + " [Thread "
              + decstr(m_tid) + "] violated by spoiler "
              + instance->running.far->regex() + " [Thread "
              + decstr(window->getTid()) + "]!\n");
          }
        }

        // All checks done
        instance->unlock();
        target->unlock();
      }
    }

    target->writelock();

    // Forget the previous target instance, replace it with a new one
    target->last.start = target->running.start;
    target->last.end = m_cvc;

    target->running.far->reset(); // Search for the next target instance
    target->running.started = false; // The instance did not start yet

    target->unlock();
  }

  for (Instances* spoiler : m_spoilers)
  { // Check if any of the spoilers can violate a target
    if (!spoiler->running.far->accepted()) continue;

    CONSOLE("Thread " + decstr(m_tid) + ": Instance of spoiler "
      + spoiler->running.far->regex() + " finished, vc.running= "
      + spoiler->running.start + ", cvc=" + m_cvc + "\n");

    for (Window* window : m_windows)
    { // For all initialised (non-NULL) threads other than this one
      if (window == NULL || window == this) continue;

      // Access information about executed target instances
      const InstancesList& targets = window->getTargets();

      for (Target::Type target : spoiler->conflicting)
      { // For each conflicting spoiler
        instance = targets[target];

        // Lock only data we will use
        spoiler->readlock();
        instance->readlock();

        if (instance->last.start.valid())
        { // If start VC is valid, end VC must also be (they are set together)
          if (!instance->last.start.hb(spoiler->running.start, m_tid)
            && !m_cvc.hb(instance->last.end, window->getTid()))
          { // start(spoiler) !< start(target) and end(target) !< end(spoiler)
            // spoiler: start=spoiler->running.start, end=m_cvc
            // target: start=instance->last.start, end=instance->last.end
            CONSOLE("Target " + spoiler->running.far->regex() + " [Thread "
              + decstr(m_tid) + "] violated by spoiler "
              + instance->running.far->regex() + " [Thread "
              + decstr(window->getTid()) + "]!\n");
          }
        }

        // All checks done
        instance->unlock();
        spoiler->unlock();
      }
    }

    spoiler->writelock();

    // Forget the previous spoiler instance, replace it with a new one
    spoiler->last.start = spoiler->running.start;
    spoiler->last.end = m_cvc;

    spoiler->running.far->reset(); // Search for the next spoiler instance
    spoiler->running.started = false; // The instance did not start yet

    spoiler->unlock();
  }
}

/** End of file window.cpp **/
