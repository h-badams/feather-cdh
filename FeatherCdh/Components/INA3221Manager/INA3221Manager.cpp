// ======================================================================
// \title  INA3221Manager.cpp
// \author john_tensorflow
// \brief  cpp file for INA3221Manager component implementation class
// ======================================================================

#include "FeatherCdh/Components/INA3221Manager/INA3221Manager.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

INA3221Manager ::INA3221Manager(const char* const compName) : INA3221ManagerComponentBase(compName) {}

INA3221Manager ::~INA3221Manager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void INA3221Manager ::schedIn_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void INA3221Manager ::FeatherCdh_INA3221StateMachine_action_doReset(SmId smId,
                                                                    FeatherCdh_INA3221StateMachine::Signal signal) {
    // TODO
}

void INA3221Manager ::FeatherCdh_INA3221StateMachine_action_checkReset(SmId smId,
                                                                       FeatherCdh_INA3221StateMachine::Signal signal) {
    // TODO
}

void INA3221Manager ::FeatherCdh_INA3221StateMachine_action_doEnable(SmId smId,
                                                                     FeatherCdh_INA3221StateMachine::Signal signal) {
    // TODO
}

void INA3221Manager ::FeatherCdh_INA3221StateMachine_action_doConfigure(SmId smId,
                                                                        FeatherCdh_INA3221StateMachine::Signal signal) {
    // TODO
}

void INA3221Manager ::FeatherCdh_INA3221StateMachine_action_doRun(SmId smId,
                                                                  FeatherCdh_INA3221StateMachine::Signal signal) {
    // TODO
}

}  // namespace FeatherCdh
