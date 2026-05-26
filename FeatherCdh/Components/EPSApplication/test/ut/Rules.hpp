#pragma once
#include "STest/Rule/Rule.hpp"
#include "TestState.hpp"

#define EPS_RULES_DEF(GROUP, RULE)                                                   \
    namespace GROUP {                                                                \
    struct RULE : public STest::Rule<EPSApplicationTestState> {                      \
        RULE() : Rule<EPSApplicationTestState>(#GROUP "." #RULE) {}                  \
        bool precondition(const EPSApplicationTestState& s) override {               \
            return s.precondition__##GROUP##__##RULE();                              \
        }                                                                            \
        void action(EPSApplicationTestState& s) override {                           \
            s.action__##GROUP##__##RULE();                                           \
        }                                                                            \
    };                                                                               \
    }

namespace FeatherCdh {
namespace EPSApplicationRules {

EPS_RULES_DEF(BatteryState, Receive)
EPS_RULES_DEF(Tick, Nominal)
EPS_RULES_DEF(Tick, LowBattery)
EPS_RULES_DEF(Tick, CriticalBattery)
EPS_RULES_DEF(Tick, StaleBattery)
EPS_RULES_DEF(SetIcRegister, Forward)
EPS_RULES_DEF(PowerStateGet, Check)

}  // namespace EPSApplicationRules
}  // namespace FeatherCdh
