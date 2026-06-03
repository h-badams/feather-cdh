// ======================================================================
// \title  SatStateMachine.cpp
// \author john_tensorflow
// \brief  cpp file for SatStateMachine component implementation class
// ======================================================================

#include "FeatherCdh/Components/SatStateMachine/SatStateMachine.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SatStateMachine ::SatStateMachine(const char* const compName) : SatStateMachineComponentBase(compName) {}

SatStateMachine ::~SatStateMachine() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SatStateMachine ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

void SatStateMachine ::schedIn_handler(FwIndexType portNum, U32 context) {
    PowerState ps = this->powerStateGet_out(0);

    Fw::ParamValid valid;
    U32 threshold = this->paramGet_CRITICAL_THRESHOLD(valid);

    U32 vbatt = ps.get_vbatt_mV();
    if (vbatt < threshold) {
        this->log_WARNING_HI_criticalBattery(vbatt);
        this->sm_sendSignal_criticalVoltage();
    }

    FeatherCdh_SatModeStateMachine::State state = this->sm_getState();
    Sat::Mode mode = (state == FeatherCdh_SatModeStateMachine::State::SAFE)
                         ? Sat::Mode::Safe
                         : Sat::Mode::Standby;
    this->tlmWrite_CURRENT_MODE(mode);
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void SatStateMachine ::SAFE_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->sm_sendSignal_safeModeCmd();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SatStateMachine ::SAFE_EXIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->sm_getState() != FeatherCdh::SatModeStateMachine_State::SAFE) {
        this->log_WARNING_LO_safeExitRejected();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->sm_sendSignal_safeExitCmd();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void SatStateMachine ::FeatherCdh_SatModeStateMachine_action_sendSafeMode(
    SmId smId,
    FeatherCdh_SatModeStateMachine::Signal signal) {
    if (this->isConnected_commsModeOut_OutputPort(0)) {
        this->commsModeOut_out(0, Comms::CommsMode::Safe);
        this->log_ACTIVITY_HI_enteredSafe();
    }
}

void SatStateMachine ::FeatherCdh_SatModeStateMachine_action_sendStandbyMode(
    SmId smId,
    FeatherCdh_SatModeStateMachine::Signal signal) {
    this->commsModeOut_out(0, Comms::CommsMode::Normal);
    this->log_ACTIVITY_HI_enteredStandby();
}

}  // namespace FeatherCdh
