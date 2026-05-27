// ======================================================================
// \title  CommsApplication.cpp
// \author john_tensorflow
// \brief  cpp file for CommsApplication component implementation class
// ======================================================================

#include "FeatherCdh/Components/CommsApplication/CommsApplication.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

CommsApplication ::CommsApplication(const char* const compName) : CommsApplicationComponentBase(compName) {}

CommsApplication ::~CommsApplication() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void CommsApplication ::commsModeIn_handler(FwIndexType portNum, const Comms::CommsMode& mode) {
    this->sm_sendSignal_switchMode(mode);
}

void CommsApplication ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

void CommsApplication ::schedIn_handler(FwIndexType portNum, U32 context) {
    this->tlmWrite_COMMS_MODE(this->m_currentMode);
    this->sm_sendSignal_tick();
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void CommsApplication ::FeatherCdh_CommsAppStateMachine_action_emitSafeMode(
    SmId smId,
    FeatherCdh_CommsAppStateMachine::Signal signal) {
    this->m_currentMode = Comms::CommsMode::Safe;
    this->m_safeDrainCounter = 0;
    this->log_ACTIVITY_HI_modeTransition(Comms::CommsMode::Safe);
}

void CommsApplication ::FeatherCdh_CommsAppStateMachine_action_emitNormalMode(
    SmId smId,
    FeatherCdh_CommsAppStateMachine::Signal signal) {
    this->m_currentMode = Comms::CommsMode::Normal;
    this->log_ACTIVITY_HI_modeTransition(Comms::CommsMode::Normal);
}

void CommsApplication ::FeatherCdh_CommsAppStateMachine_action_doSafeDrain(
    SmId smId,
    FeatherCdh_CommsAppStateMachine::Signal signal) {
    Fw::ParamValid valid;
    const U32 divisor = this->paramGet_SAFE_DRAIN_DIVISOR(valid);
    this->m_safeDrainCounter++;
    if (this->m_safeDrainCounter >= divisor) {
        this->comQueueRun_out(0, 0);
        this->m_safeDrainCounter = 0;
    }
}

void CommsApplication ::FeatherCdh_CommsAppStateMachine_action_doNormalDrain(
    SmId smId,
    FeatherCdh_CommsAppStateMachine::Signal signal) {
    this->comQueueRun_out(0, 0);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool CommsApplication ::FeatherCdh_CommsAppStateMachine_guard_isModeNormal(
    SmId smId,
    FeatherCdh_CommsAppStateMachine::Signal signal,
    const Comms::CommsMode& value) const {
    return value == Comms::CommsMode::Normal;
}

bool CommsApplication ::FeatherCdh_CommsAppStateMachine_guard_isModeSafe(SmId smId,
                                                                         FeatherCdh_CommsAppStateMachine::Signal signal,
                                                                         const Comms::CommsMode& value) const {
    return value == Comms::CommsMode::Safe;
}

}  // namespace FeatherCdh
