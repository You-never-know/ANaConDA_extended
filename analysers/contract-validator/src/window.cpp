/**
 * @brief Contains implementation of a class representing a window.
 *
 * A file containing implementation of a class representing a window.
 *
 * @file      window.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2016-02-23
 * @date      Last Update 2016-02-24
 * @version   0.2
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
    e->far->advance(name);
  }

  for (Element* e : m_spoilers)
  { // Try to advance all spoilers
    e->far->advance(name);
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

    CONSOLE("Instance of target " + e->far->regex() + " finished.\n");

    e->far->reset(); // Search for the next target instance
  }

  for (Element* e : m_spoilers)
  { // Check if any of the spoilers can violate a target
    if (!e->far->accepted()) continue;

    CONSOLE("Instance of spoiler " + e->far->regex() + " finished.\n");

    e->far->reset(); // Search for the next spoiler instance
  }
}

/** End of file window.cpp **/
