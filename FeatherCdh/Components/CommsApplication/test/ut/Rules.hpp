#pragma once
#include "STest/Rule/Rule.hpp"
#include "TestState.hpp"

#define COMMS_APP_RULES_DEF(GROUP, RULE)                                       \
    namespace GROUP {                                                          \
    struct RULE : public STest::Rule<CommsApplicationTestState> {              \
        RULE() : Rule<CommsApplicationTestState>(#GROUP "." #RULE) {}          \
        bool precondition(const CommsApplicationTestState& s) override {       \
            return s.precondition__##GROUP##__##RULE();                        \
        }                                                                      \
        void action(CommsApplicationTestState& s) override {                   \
            s.action__##GROUP##__##RULE();                                     \
        }                                                                      \
    };                                                                         \
    }

namespace FeatherCdh {
namespace CommsApplicationRules {

COMMS_APP_RULES_DEF(Sm, SafeOnBoot)
COMMS_APP_RULES_DEF(Sm, SafeToNormal)
COMMS_APP_RULES_DEF(Sm, NormalToSafe)
COMMS_APP_RULES_DEF(Sm, SafeIgnoresSafe)
COMMS_APP_RULES_DEF(Sm, NormalIgnoresNormal)

COMMS_APP_RULES_DEF(Drain, NormalEveryTick)
COMMS_APP_RULES_DEF(Drain, SafeEveryDivisor)
COMMS_APP_RULES_DEF(Drain, SafeSuppressesBetween)
COMMS_APP_RULES_DEF(Drain, ModeSwitchResetsCounter)

COMMS_APP_RULES_DEF(Tlm, CommsModeEachTick)

COMMS_APP_RULES_DEF(Ping, Echo)

}  // namespace CommsApplicationRules
}  // namespace FeatherCdh
