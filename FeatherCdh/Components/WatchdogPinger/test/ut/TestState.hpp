#pragma once
#include "WatchdogPingerTester.hpp"

namespace FeatherCdh {

class WatchdogPingerTestState : public WatchdogPingerTester {
  public:
    bool precondition__Ping__Nominal() const;
    void action__Ping__Nominal();
};

}  // namespace FeatherCdh
