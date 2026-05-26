#include "TestState.hpp"

namespace FeatherCdh {

bool WatchdogPingerTestState::precondition__Ping__Nominal() const {
    return true;
}

void WatchdogPingerTestState::action__Ping__Nominal() {
    this->clearHistory();

    // Passive component: schedIn is sync, handler executes immediately
    this->invoke_to_schedIn(0, 0);

    // Expect exactly two GPIO calls: HIGH then LOW
    ASSERT_from_watchdogPing_SIZE(2);
    ASSERT_from_watchdogPing(0, Fw::Logic::HIGH);
    ASSERT_from_watchdogPing(1, Fw::Logic::LOW);
}

}  // namespace FeatherCdh
