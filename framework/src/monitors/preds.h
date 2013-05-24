/**
 * @brief Contains definitions of classes monitoring predecessors.
 *
 * A file containing definitions of classes monitoring predecessors.
 *
 * @file      preds.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-04-05
 * @date      Last Update 2013-04-16
 * @version   0.1
 */

#ifndef __PINTOOL_ANACONDA__COVERAGE__PREDS_H__
  #define __PINTOOL_ANACONDA__COVERAGE__PREDS_H__

#include <set>

#include "pin.H"

#include "../types.h"

/**
 * @brief A class monitoring predecessors.
 *
 * Monitors predecessors.
 *
 * @tparam Writer A writer which should be used to write all the information
 *   about the predecessors.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-04-05
 * @date      Last Update 2013-04-16
 * @version   0.1
 */
template< typename Writer >
class PredecessorsMonitor : public Writer
{
  private: // Type definitions
    typedef std::set< ADDRINT > PredecessorSet;
  private: // Internal variables
    PredecessorSet m_pSet; //!< A set containing instructions with predecessors.
    PIN_RWMUTEX m_pSetLock; //!< A lock guarding access to the predecessor set.
  public: // Constructors
    /**
     * Constructs a PredecessorsMonitor object.
     */
    PredecessorsMonitor() { PIN_RWMutexInit(&m_pSetLock); }
  public: // Destructors
    ~PredecessorsMonitor();
  public: // Static methods
    static VOID initTls(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
  public: // Methods for loading predecessors
    void load(const std::string& path);
  public: // Methods monitoring the predecessors
    void beforeFunctionEntered(THREADID tid);
    void beforeFunctionExited(THREADID tid);
    void beforeVariableAccessed(THREADID tid, ADDRINT ins, VARIABLE var);
  public: // Methods for checking instructions
    bool hasPredecessor(ADDRINT ins);
};

#endif /* __PINTOOL_ANACONDA__COVERAGE__PREDS_H__ */

/** End of file preds.h **/
