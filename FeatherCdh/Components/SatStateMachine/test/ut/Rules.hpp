#pragma once
#include "STest/Rule/Rule.hpp"
#include "TestState.hpp"

#define SSM_RULES_DEF(GROUP, RULE)                                                       \
    namespace GROUP {                                                                    \
    struct RULE : public STest::Rule<SatStateMachineTestState> {                         \
        RULE() : Rule<SatStateMachineTestState>(#GROUP "." #RULE) {}                     \
        bool precondition(const SatStateMachineTestState& s) override {                  \
            return s.precondition__##GROUP##__##RULE();                                  \
        }                                                                                \
        void action(SatStateMachineTestState& s) override {                              \
            s.action__##GROUP##__##RULE();                                               \
        }                                                                                \
    };                                                                                   \
    }

namespace FeatherCdh {
namespace SatStateMachineRules {

SSM_RULES_DEF(Tick, InSafeNominal)
SSM_RULES_DEF(Tick, InSafeCritical)
SSM_RULES_DEF(Tick, InStandbyNominal)
SSM_RULES_DEF(Tick, InStandbyCritical)
SSM_RULES_DEF(Cmd, SafeMode)
SSM_RULES_DEF(Cmd, SafeExit)
SSM_RULES_DEF(Cmd, SafeExitRejected)
SSM_RULES_DEF(Ping, Nominal)

}  // namespace SatStateMachineRules
}  // namespace FeatherCdh
