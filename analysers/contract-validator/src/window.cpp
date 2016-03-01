/**
 * @brief Contains implementation of a class representing a window.
 *
 * A file containing implementation of a class representing a window.
 *
 * @file      window.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-03-01
 * @version   0.8
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
  for (Instances* instance : m_targets)
  { // Try to advance all targets
    this->advance(instance, name);
  }

  for (Instances* instance : m_spoilers)
  { // Try to advance all spoilers
    this->advance(instance, name);
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
            this->reportViolation(target, m_tid, instance, window->getTid());
          }
        }

        // All checks done
        instance->unlock();
        target->unlock();
      }
    }

    this->replaceLast(target); // v -> r_old
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
            this->reportViolation(instance, window->getTid(), spoiler, m_tid);
          }
        }

        // All checks done
        instance->unlock();
        spoiler->unlock();
      }
    }

    this->replaceLast(spoiler); // v -> s_old
  }
}

/**
 * Tries to advance the currently running instance of a target or a spoiler. If
 *   no instance of this target or spoiler is currently running, tries to start
 *   a new instance starting with the function encountered in the execution.
 *
 * @param instance A structure containing information about the currently
 *   running instance.
 * @param name A name of the function encountered in the execution.
 */
void Window::advance(Instances* instance, const std::string& name)
{
  switch (instance->running.far->advance(name))
  { // Try to advance the currently running instance
    case FARunner::MOVED_TO_NEXT_STATE:
      if (!instance->running.started)
      { // We encountered a start of a new instance
        instance->running.started = true;
        instance->running.start = m_cvc;
      }
      break;
    case FARunner::NO_TRANSITION_FOUND:
      // We just invalidated the running instance
      instance->running.started = false;
      instance->running.far->reset();
      break;
    case FARunner::INVALID_SYMBOL:
      break;
  }
}

/**
 * Replaces the last encountered (valid) instance with a newly encountered one.
 *
 * @param instance A structure containing information about the last and newly
 *   encountered instances.
 */
void Window::replaceLast(Instances* instance)
{
  // This has to be done exclusively as other threads may read this information
  instance->writelock();

  // Forget the last instance and replace it with the new one
  instance->last.start = instance->running.start;
  instance->last.end = m_cvc;

  // There is no running instance now
  instance->running.started = false;
  instance->running.far->reset();

  // The state is consistent, allow the other threads to read this information
  instance->unlock();
}

/**
 * Prints information about a detected contract violation.
 *
 * @param target A structure containing information about the target whose
 *   instance was violated by a spoiler instance.
 * @param ttid A number identifying a thread which executed the instance of the
 *   target.
 * @param spoiler A structure containing information about the spoiler whose
 *   instance violated a target instance.
 * @param stid A number identifying a thread which executed the instance of the
 *   spoiler.
 */
void Window::reportViolation(Instances* target, THREADID ttid,
  Instances* spoiler, THREADID stid)
{
  CONSOLE(std::string("Contract violation detected!\n")
    + "  Target [Thread " + decstr(ttid) + "]: "
    + target->running.far->regex() + "\n"
    + "  Spoiler [Thread " + decstr(stid) + "]: "
    + spoiler->running.far->regex() + "\n");
}

/** End of file window.cpp **/
