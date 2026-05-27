#pragma once
#include "STest/Rule/Rule.hpp"
#include "TestState.hpp"

#define HC12_RULES_DEF(GROUP, RULE)                                                \
    namespace GROUP {                                                              \
    struct RULE : public STest::Rule<HC12ManagerTestState> {                       \
        RULE() : Rule<HC12ManagerTestState>(#GROUP "." #RULE) {}                   \
        bool precondition(const HC12ManagerTestState& s) override {                \
            return s.precondition__##GROUP##__##RULE();                            \
        }                                                                          \
        void action(HC12ManagerTestState& s) override {                            \
            s.action__##GROUP##__##RULE();                                         \
        }                                                                          \
    };                                                                             \
    }

namespace FeatherCdh {
namespace HC12ManagerRules {

HC12_RULES_DEF(Sm, ResetTick)
HC12_RULES_DEF(Sm, WaitResetTick)
HC12_RULES_DEF(Sm, ConfigureNominal)
HC12_RULES_DEF(Sm, RunTickTelemetry)

HC12_RULES_DEF(Downlink, Forward)
HC12_RULES_DEF(Downlink, UartError)

HC12_RULES_DEF(Uplink, Forward)
HC12_RULES_DEF(Uplink, ReceiveError)
HC12_RULES_DEF(Uplink, ReturnBuffer)

HC12_RULES_DEF(DrvConnected, NoOp)

}  // namespace HC12ManagerRules
}  // namespace FeatherCdh
