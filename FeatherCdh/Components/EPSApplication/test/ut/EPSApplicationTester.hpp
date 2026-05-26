#pragma once
#include "EPSApplicationGTestBase.hpp"
#include "FeatherCdh/Components/EPSApplication/EPSApplication.hpp"

namespace FeatherCdh {

class EPSApplicationTester : public EPSApplicationGTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static constexpr U32 MAX_HISTORY_SIZE = 32;

    EPSApplication component;

    // Injectable state
    U32 m_powerThreshold = 0;
    U32 m_criticalThreshold = 0;
    BQ25756Reg m_setRegAddr = BQ25756Reg::CHARGE_VOLT_LIM;
    U32 m_setRegValue = 0;
    bool m_setRegCalled = false;

    EPSApplicationTester()
        : EPSApplicationGTestBase("EPSApplicationTester", MAX_HISTORY_SIZE),
          component("EPSApplication") {
        this->initComponents();
        this->connectPorts();
        this->clearHistory();
    }

    // Bodies provided by UT_AUTO_HELPERS
    void connectPorts();
    void initComponents();

    // Helpers used by rules
    void sendBatteryState(const BatteryState& bs) {
        this->invoke_to_batteryStateIn(0, const_cast<BatteryState&>(bs));
        EPSApplicationTesterBase::dispatchOne(this->component);
    }

    void sendSchedTick() {
        this->invoke_to_schedIn(0, 0);
        EPSApplicationTesterBase::dispatchOne(this->component);
    }

    void setParams() {
        this->paramSet_POWER_THRESHOLD(m_powerThreshold, Fw::ParamValid::VALID);
        this->paramSet_CRITICAL_THRESHOLD(m_criticalThreshold, Fw::ParamValid::VALID);
        this->component.loadParameters();
    }

  protected:
    void from_setRegister_handler(FwIndexType portNum,
                                  const BQ25756Reg& regAddr, U32 value) override {
        this->pushFromPortEntry_setRegister(regAddr, value);
        m_setRegAddr = regAddr;
        m_setRegValue = value;
        m_setRegCalled = true;
    }

    void from_pingOut_handler(FwIndexType portNum, U32 key) override {
        this->pushFromPortEntry_pingOut(key);
    }
};

}  // namespace FeatherCdh
