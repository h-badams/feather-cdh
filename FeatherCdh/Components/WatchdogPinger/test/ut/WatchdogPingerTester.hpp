#pragma once
#include "WatchdogPingerGTestBase.hpp"
#include "FeatherCdh/Components/WatchdogPinger/WatchdogPinger.hpp"

namespace FeatherCdh {

class WatchdogPingerTester : public WatchdogPingerGTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr U32 MAX_HISTORY_SIZE = 16;

    WatchdogPinger component;
    Drv::GpioStatus m_gpioStatus = Drv::GpioStatus::OP_OK;

    WatchdogPingerTester()
        : WatchdogPingerGTestBase("WatchdogPingerTester", MAX_HISTORY_SIZE),
          component("WatchdogPinger") {
        this->initComponents();
        this->connectPorts();
        this->clearHistory();
    }

    // Bodies provided by UT_AUTO_HELPERS (WatchdogPingerTesterHelpers.cpp)
    void connectPorts();
    void initComponents();

  protected:
    Drv::GpioStatus from_watchdogPing_handler(FwIndexType portNum,
                                              const Fw::Logic& state) override {
        this->pushFromPortEntry_watchdogPing(state);
        return this->m_gpioStatus;
    }
};

}  // namespace FeatherCdh
