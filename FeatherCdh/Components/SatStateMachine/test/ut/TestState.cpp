#include "TestState.hpp"

namespace FeatherCdh {

// ---------------------------------------------------------------------------
// Rule: Tick::InSafeNominal
//   Precondition: SM is in SAFE
//   Action: tick with vbatt well above CRITICAL_THRESHOLD; no events,
//           no mode command, CURRENT_MODE telemetry reports Safe
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Tick__InSafeNominal() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::SAFE;
}

void SatStateMachineTestState::action__Tick__InSafeNominal() {
    this->clearHistory();
    setThreshold(10000);

    m_mockPowerState.set_vbatt_mV(21000);
    m_mockPowerState.set_chargingState(BQ25756EChargingState::TAPER_CHARGE_CV);
    m_mockPowerState.set_faultStatus(0);

    sendTick();

    ASSERT_EVENTS_criticalBattery_SIZE(0);
    ASSERT_EVENTS_enteredSafe_SIZE(0);
    ASSERT_EVENTS_enteredStandby_SIZE(0);
    ASSERT_from_commsModeOut_SIZE(0);
    ASSERT_from_powerStateGet_SIZE(1);
    ASSERT_TLM_CURRENT_MODE_SIZE(1);
    ASSERT_TLM_CURRENT_MODE(0, Sat::Mode::Safe);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::SAFE);
}

// ---------------------------------------------------------------------------
// Rule: Tick::InSafeCritical
//   Precondition: SM is in SAFE
//   Action: tick with vbatt below CRITICAL_THRESHOLD; criticalBattery event
//           fires and criticalVoltage signal is sent, but SAFE has no handler
//           for that signal in the FPP — SM stays SAFE, no commsModeOut
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Tick__InSafeCritical() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::SAFE;
}

void SatStateMachineTestState::action__Tick__InSafeCritical() {
    this->clearHistory();
    setThreshold(10000);

    m_mockPowerState.set_vbatt_mV(4000);
    m_mockPowerState.set_chargingState(BQ25756EChargingState::NOT_CHARGING);
    m_mockPowerState.set_faultStatus(0);

    sendTick();

    ASSERT_EVENTS_criticalBattery_SIZE(1);
    ASSERT_EVENTS_criticalBattery(0, static_cast<U32>(4000));

    // SAFE has no criticalVoltage handler — no re-entry, no commsModeOut
    ASSERT_EVENTS_enteredSafe_SIZE(0);
    ASSERT_EVENTS_enteredStandby_SIZE(0);
    ASSERT_from_commsModeOut_SIZE(0);

    ASSERT_TLM_CURRENT_MODE_SIZE(1);
    ASSERT_TLM_CURRENT_MODE(0, Sat::Mode::Safe);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::SAFE);
}

// ---------------------------------------------------------------------------
// Rule: Tick::InStandbyNominal
//   Precondition: SM is in STANDBY
//   Action: tick with vbatt above threshold; no events, no mode command,
//           CURRENT_MODE telemetry reports Standby
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Tick__InStandbyNominal() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::STANDBY;
}

void SatStateMachineTestState::action__Tick__InStandbyNominal() {
    this->clearHistory();
    setThreshold(10000);

    m_mockPowerState.set_vbatt_mV(21000);
    m_mockPowerState.set_chargingState(BQ25756EChargingState::TAPER_CHARGE_CV);
    m_mockPowerState.set_faultStatus(0);

    sendTick();

    ASSERT_EVENTS_criticalBattery_SIZE(0);
    ASSERT_EVENTS_enteredSafe_SIZE(0);
    ASSERT_EVENTS_enteredStandby_SIZE(0);
    ASSERT_from_commsModeOut_SIZE(0);
    ASSERT_from_powerStateGet_SIZE(1);
    ASSERT_TLM_CURRENT_MODE_SIZE(1);
    ASSERT_TLM_CURRENT_MODE(0, Sat::Mode::Standby);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::STANDBY);
}

// ---------------------------------------------------------------------------
// Rule: Tick::InStandbyCritical
//   Precondition: SM is in STANDBY
//   Action: tick with vbatt below threshold; criticalBattery event fires,
//           criticalVoltage signal triggers STANDBY→SAFE transition,
//           enteredSafe event and commsModeOut(Safe) are emitted.
//           CURRENT_MODE telemetry reflects the SM state at time of the
//           schedIn handler (before the SM signal is dispatched → Standby).
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Tick__InStandbyCritical() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::STANDBY;
}

void SatStateMachineTestState::action__Tick__InStandbyCritical() {
    this->clearHistory();
    setThreshold(10000);

    m_mockPowerState.set_vbatt_mV(3000);
    m_mockPowerState.set_chargingState(BQ25756EChargingState::NOT_CHARGING);
    m_mockPowerState.set_faultStatus(0);

    sendTick();

    ASSERT_EVENTS_criticalBattery_SIZE(1);
    ASSERT_EVENTS_criticalBattery(0, static_cast<U32>(3000));
    ASSERT_EVENTS_enteredSafe_SIZE(1);
    ASSERT_from_commsModeOut_SIZE(1);
    ASSERT_from_commsModeOut(0, Comms::CommsMode::Safe);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::SAFE);
}

// ---------------------------------------------------------------------------
// Rule: Cmd::SafeMode
//   Precondition: SM is in STANDBY (so the transition is observable)
//   Action: SAFE_MODE command → STANDBY→SAFE, enteredSafe event,
//           commsModeOut(Safe), CMD OK
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Cmd__SafeMode() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::STANDBY;
}

void SatStateMachineTestState::action__Cmd__SafeMode() {
    this->clearHistory();

    this->sendCmd_SAFE_MODE(TEST_INSTANCE_ID, 1);
    SatStateMachineTesterBase::dispatchCurrentMessages(this->component);

    ASSERT_EVENTS_enteredSafe_SIZE(1);
    ASSERT_from_commsModeOut_SIZE(1);
    ASSERT_from_commsModeOut(0, Comms::CommsMode::Safe);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::SAFE);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, OPCODE_SAFE_MODE, 1, Fw::CmdResponse::OK);
}

// ---------------------------------------------------------------------------
// Rule: Cmd::SafeExit
//   Precondition: SM is in SAFE
//   Action: SAFE_EXIT command → SAFE→STANDBY, enteredStandby event,
//           commsModeOut(Normal), CMD OK
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Cmd__SafeExit() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::SAFE;
}

void SatStateMachineTestState::action__Cmd__SafeExit() {
    this->clearHistory();

    this->sendCmd_SAFE_EXIT(TEST_INSTANCE_ID, 1);
    SatStateMachineTesterBase::dispatchCurrentMessages(this->component);

    ASSERT_EVENTS_enteredStandby_SIZE(1);
    ASSERT_from_commsModeOut_SIZE(1);
    ASSERT_from_commsModeOut(0, Comms::CommsMode::Normal);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::STANDBY);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, OPCODE_SAFE_EXIT, 1, Fw::CmdResponse::OK);
}

// ---------------------------------------------------------------------------
// Rule: Cmd::SafeExitRejected
//   Precondition: SM is in STANDBY (SAFE_EXIT invalid outside SAFE)
//   Action: SAFE_EXIT command → safeExitRejected event, CMD EXECUTION_ERROR,
//           no mode command sent, SM remains in STANDBY
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Cmd__SafeExitRejected() const {
    return this->component.getSmState() == FeatherCdh::SatModeStateMachine_State::STANDBY;
}

void SatStateMachineTestState::action__Cmd__SafeExitRejected() {
    this->clearHistory();

    this->sendCmd_SAFE_EXIT(TEST_INSTANCE_ID, 1);
    SatStateMachineTesterBase::dispatchCurrentMessages(this->component);

    ASSERT_EVENTS_safeExitRejected_SIZE(1);
    ASSERT_EVENTS_enteredSafe_SIZE(0);
    ASSERT_from_commsModeOut_SIZE(0);
    ASSERT_EQ(this->component.getSmState(), FeatherCdh::SatModeStateMachine_State::STANDBY);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, OPCODE_SAFE_EXIT, 1, Fw::CmdResponse::EXECUTION_ERROR);
}

// ---------------------------------------------------------------------------
// Rule: Ping::Nominal
//   Precondition: always
//   Action: pingIn → pingOut carries the same key (health liveness check)
// ---------------------------------------------------------------------------

bool SatStateMachineTestState::precondition__Ping__Nominal() const {
    return true;
}

void SatStateMachineTestState::action__Ping__Nominal() {
    this->clearHistory();

    this->invoke_to_pingIn(0, 42);
    SatStateMachineTesterBase::dispatchOne(this->component);

    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, static_cast<U32>(42));
}

}  // namespace FeatherCdh
