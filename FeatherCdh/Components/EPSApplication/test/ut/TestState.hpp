#pragma once
#include "EPSApplicationTester.hpp"

namespace FeatherCdh {

class EPSApplicationTestState : public EPSApplicationTester {
  public:
    bool precondition__BatteryState__Receive() const;
    void action__BatteryState__Receive();

    bool precondition__Tick__Nominal() const;
    void action__Tick__Nominal();

    bool precondition__Tick__LowBattery() const;
    void action__Tick__LowBattery();

    bool precondition__Tick__CriticalBattery() const;
    void action__Tick__CriticalBattery();

    bool precondition__Tick__StaleBattery() const;
    void action__Tick__StaleBattery();

    bool precondition__SetIcRegister__Forward() const;
    void action__SetIcRegister__Forward();

    bool precondition__PowerStateGet__Check() const;
    void action__PowerStateGet__Check();
};

}  // namespace FeatherCdh
