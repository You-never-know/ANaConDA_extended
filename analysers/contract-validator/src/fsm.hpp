/**
 * @brief Contains implementation of a class representing a finite state
 *   machine (FSM).
 *
 * A file containing implementation of a class representing a finite state
 *   machine (FSM).
 *
 * @file      fsm.hpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-27
 * @version   0.1
 */

#ifndef __FSM_HPP__
  #define __FSM_HPP__

/**
 * @brief An enumeration of types of states which might be present in a finite
 *   state machine (FSM).
 */
typedef enum FsmStateType_e
{
  NORMAL,    //!< A normal state.
  STARTING,  //!< A starting state.
  ACCEPTING, //!< An accepting state.
} FsmStateType;

/**
 * @brief A structure representing a state of a finite state machine (FSM).
 */
typedef struct FsmState_s
{
  /**
   * @brief A list of outgoing transitions.
   */
  std::map< std::string, FsmState_s* > transitions;
  FsmStateType type; //!< A type of the state.
} FsmState;

/**
 * @brief A class representing a finite state machine (FSM).
 *
 * Represents a finite state machine (FSM).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2014-11-27
 * @date      Last Update 2014-11-27
 * @version   0.1
 */
class Fsm
{
  private:
    FsmState* m_start;
    FsmState* m_current;
  public:
    bool advance(std::string symbol) { return false; }
};

#endif /* __FSM_HPP__ */

/** End of file fsm.hpp **/
