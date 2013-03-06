/**
 * @brief Contains definitions of classes monitoring shared variables.
 *
 * A file containing definitions of classes monitoring shared variables.
 *
 * @file      svars.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-03-06
 * @version   0.2
 */

#ifndef __PINTOOL_ANACONDA__COVERAGE__SVARS_H__
  #define __PINTOOL_ANACONDA__COVERAGE__SVARS_H__

#include <map>
#include <set>

#include "pin.H"

#include "../types.h"

/**
 * @brief A class monitoring shared variables.
 *
 * Monitors shared variables.
 *
 * @tparam Writer A writer which should be used to write all the information
 *   about the shared variables.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-02-26
 * @date      Last Update 2013-03-06
 * @version   0.2
 */
template< typename Writer >
class SharedVarsMonitor : public Writer
{
  private: // Type definitions
    typedef std::map< std::string, std::set< THREADID > > VarMap;
  private: // Internal variables
    VarMap m_varMap; //!< A map containing information about variables.
    PIN_RWMUTEX m_varMapLock; //!< A lock guarding access to the variable map.
  public: // Constructors
    /**
     * Constructs a SharedVarsMonitor object.
     */
    SharedVarsMonitor() { PIN_RWMutexInit(&m_varMapLock); }
  public: // Destructors
    ~SharedVarsMonitor();
  public: // Methods monitoring the shared variables
    void beforeVariableAccessed(THREADID tid, VARIABLE var);
  public: // Methods for checking variables
    bool isSharedVariable(VARIABLE var);
};

#endif /* __PINTOOL_ANACONDA__COVERAGE__SVARS_H__ */

/** End of file svars.h **/
