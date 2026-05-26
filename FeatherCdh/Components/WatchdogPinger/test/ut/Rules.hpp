#pragma once
#include "STest/Rule/Rule.hpp"
#include "TestState.hpp"

// Each macro expands to an STest::Rule<WatchdogPingerTestState> struct whose
// precondition/action delegate to the correspondingly named methods on the
// state object (which derives from GTestBase and can use ASSERT_* macros).
#define WATCHDOG_RULES_DEF(GROUP, RULE)                                                \
    namespace GROUP {                                                                  \
    struct RULE : public STest::Rule<WatchdogPingerTestState> {                        \
        RULE() : Rule<WatchdogPingerTestState>(#GROUP "." #RULE) {}                    \
        bool precondition(const WatchdogPingerTestState& s) override {                 \
            return s.precondition__##GROUP##__##RULE();                                \
        }                                                                              \
        void action(WatchdogPingerTestState& s) override {                             \
            s.action__##GROUP##__##RULE();                                             \
        }                                                                              \
    };                                                                                 \
    }

namespace FeatherCdh {
namespace WatchdogPingerRules {

WATCHDOG_RULES_DEF(Ping, Nominal)

}  // namespace WatchdogPingerRules
}  // namespace FeatherCdh
