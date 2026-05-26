#include "TestState.hpp"

namespace FeatherCdh {

// ---------------------------------------------------------------------------
// Rule: BatteryState::Receive
//   batteryStateIn is cache-only — no events on receipt
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__BatteryState__Receive() const {
    return true;
}

void EPSApplicationTestState::action__BatteryState__Receive() {
    this->clearHistory();

    BatteryState bs;
    bs.set_vbatt_mV(12000);
    bs.set_chargingState(BQ25756EChargingState::FAST_CHARGE_CC);
    bs.set_faultStatus(0);
    sendBatteryState(bs);

    ASSERT_EVENTS_SIZE(0);
}

// ---------------------------------------------------------------------------
// Rule: Tick::Nominal
//   vbatt well above thresholds → no warning events
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__Tick__Nominal() const {
    return true;
}

void EPSApplicationTestState::action__Tick__Nominal() {
    this->clearHistory();
    m_powerThreshold = 0;
    m_criticalThreshold = 0;
    setParams();

    BatteryState bs;
    bs.set_vbatt_mV(15000);
    sendBatteryState(bs);
    sendSchedTick();

    ASSERT_EVENTS_SIZE(0);
    ASSERT_EVENTS_lowBattery_SIZE(0);
    ASSERT_EVENTS_criticalBattery_SIZE(0);
}

// ---------------------------------------------------------------------------
// Rule: Tick::LowBattery
//   vbatt below POWER_THRESHOLD but above CRITICAL_THRESHOLD → lowBattery only
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__Tick__LowBattery() const {
    return true;
}

void EPSApplicationTestState::action__Tick__LowBattery() {
    this->clearHistory();
    m_powerThreshold = 10000;
    m_criticalThreshold = 5000;
    setParams();

    BatteryState bs;
    bs.set_vbatt_mV(8000);
    sendBatteryState(bs);
    sendSchedTick();

    ASSERT_EVENTS_lowBattery_SIZE(1);
    ASSERT_EVENTS_lowBattery(0, 8000);
    ASSERT_EVENTS_criticalBattery_SIZE(0);
}

// ---------------------------------------------------------------------------
// Rule: Tick::CriticalBattery
//   vbatt below both thresholds → both events fire
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__Tick__CriticalBattery() const {
    return true;
}

void EPSApplicationTestState::action__Tick__CriticalBattery() {
    this->clearHistory();
    m_powerThreshold = 10000;
    m_criticalThreshold = 5000;
    setParams();

    BatteryState bs;
    bs.set_vbatt_mV(3000);
    sendBatteryState(bs);
    sendSchedTick();

    ASSERT_EVENTS_lowBattery_SIZE(1);
    ASSERT_EVENTS_lowBattery(0, 3000);
    ASSERT_EVENTS_criticalBattery_SIZE(1);
    ASSERT_EVENTS_criticalBattery(0, 3000);
}

// ---------------------------------------------------------------------------
// Rule: Tick::StaleBattery
//   Tick with no prior batteryStateIn (default 0 mV, thresholds 0) → no events
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__Tick__StaleBattery() const {
    return true;
}

void EPSApplicationTestState::action__Tick__StaleBattery() {
    EPSApplicationTestState freshState;
    freshState.clearHistory();
    freshState.invoke_to_schedIn(0, 0);
    EPSApplicationTesterBase::dispatchOne(freshState.component);
    // 0 mV is not < 0 threshold
    ASSERT_EVENTS_SIZE(0);
}

// ---------------------------------------------------------------------------
// Rule: SetIcRegister::Forward
//   Command → setRegister output port called + icRegisterForwarded event + CMD_OK
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__SetIcRegister__Forward() const {
    return true;
}

void EPSApplicationTestState::action__SetIcRegister__Forward() {
    this->clearHistory();
    m_setRegCalled = false;

    this->sendCmd_SET_IC_REGISTER(TEST_INSTANCE_ID, 1, BQ25756Reg::MPPT_CONT, 0x01);
    EPSApplicationTesterBase::dispatchOne(this->component);

    ASSERT_from_setRegister_SIZE(1);
    ASSERT_from_setRegister(0, BQ25756Reg::MPPT_CONT, static_cast<U32>(0x01));
    ASSERT_TRUE(m_setRegCalled);
    ASSERT_EQ(m_setRegAddr, BQ25756Reg::MPPT_CONT);
    ASSERT_EQ(m_setRegValue, static_cast<U32>(0x01));
    ASSERT_EVENTS_icRegisterForwarded_SIZE(1);
    ASSERT_EVENTS_icRegisterForwarded(0, BQ25756Reg::MPPT_CONT, static_cast<U32>(0x01));
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, 0x0, 1,
                        Fw::CmdResponse::OK);
}

// ---------------------------------------------------------------------------
// Rule: PowerStateGet::Check
//   After receive+tick, powerStateGet returns cached PowerState with correct fields
// ---------------------------------------------------------------------------

bool EPSApplicationTestState::precondition__PowerStateGet__Check() const {
    return true;
}

void EPSApplicationTestState::action__PowerStateGet__Check() {
    this->clearHistory();
    m_powerThreshold = 0;
    m_criticalThreshold = 0;
    setParams();

    BatteryState bs;
    bs.set_vbatt_mV(21000);
    bs.set_chargingState(BQ25756EChargingState::TAPER_CHARGE_CV);
    bs.set_faultStatus(0x00);
    sendBatteryState(bs);
    sendSchedTick();

    PowerState ps = this->invoke_to_powerStateGet(0);

    ASSERT_EQ(ps.get_vbatt_mV(), static_cast<U32>(21000));
    ASSERT_EQ(ps.get_chargingState(), BQ25756EChargingState::TAPER_CHARGE_CV);
    ASSERT_EQ(ps.get_faultStatus(), static_cast<U8>(0x00));
}

}  // namespace FeatherCdh
