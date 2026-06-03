// ======================================================================
// \title  MpptIcManager.cpp
// \author john_tensorflow
// \brief  cpp file for MpptIcManager component implementation class
// ======================================================================

#include "FeatherCdh/Components/MpptIcManager/MpptIcManager.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

MpptIcManager ::MpptIcManager(const char* const compName) : MpptIcManagerComponentBase(compName) {}

MpptIcManager ::~MpptIcManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void MpptIcManager ::intPin_handler(FwIndexType portNum, Os::RawTime& cycleStart) {
    // TODO
}

void MpptIcManager ::schedIn_handler(FwIndexType portNum, U32 context) {
    // TODO
}

void MpptIcManager ::setRegister_handler(FwIndexType portNum, const FeatherCdh::BQ25756Reg& regAddr, U32 value) {
    // TODO
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void MpptIcManager ::FeatherCdh_MpptIcStateMachine_action_doReset(SmId smId,
                                                                  FeatherCdh_MpptIcStateMachine::Signal signal) {
    // TODO
}

void MpptIcManager ::FeatherCdh_MpptIcStateMachine_action_checkReset(SmId smId,
                                                                     FeatherCdh_MpptIcStateMachine::Signal signal) {
    // TODO
}

void MpptIcManager ::FeatherCdh_MpptIcStateMachine_action_doEnable(SmId smId,
                                                                   FeatherCdh_MpptIcStateMachine::Signal signal) {
    // TODO
}

void MpptIcManager ::FeatherCdh_MpptIcStateMachine_action_doConfigure(SmId smId,
                                                                      FeatherCdh_MpptIcStateMachine::Signal signal) {
    // TODO
}

void MpptIcManager ::FeatherCdh_MpptIcStateMachine_action_doRun(SmId smId,
                                                                FeatherCdh_MpptIcStateMachine::Signal signal) {
    // TODO
}

}  // namespace FeatherCdh
