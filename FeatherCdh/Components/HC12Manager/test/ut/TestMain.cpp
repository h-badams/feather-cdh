// F Prime's Os::Generic::PriorityQueue allocates queue storage via MallocAllocator
// and never frees it in the destructor (by design for embedded targets). Disable
// LSan to suppress these false-positive leaks from the framework.
extern "C" int __lsan_is_turned_off() { return 1; }

#include <gtest/gtest.h>
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"
#include "STest/Random/Random.hpp"
#include "Rules.hpp"

using namespace FeatherCdh;
using namespace FeatherCdh::HC12ManagerRules;

// ---------------------------------------------------------------------------
// Individual GTest cases (one rule per case, deterministic)
// ---------------------------------------------------------------------------

TEST(HC12Manager, SmResetTick) {
    HC12ManagerTestState state;
    Sm::ResetTick rule;
    rule.apply(state);
}

TEST(HC12Manager, SmWaitResetTick) {
    HC12ManagerTestState state;
    Sm::WaitResetTick rule;
    rule.apply(state);
}

TEST(HC12Manager, SmConfigureNominal) {
    HC12ManagerTestState state;
    Sm::ConfigureNominal rule;
    rule.apply(state);
}

TEST(HC12Manager, SmRunTickTelemetry) {
    HC12ManagerTestState state;
    Sm::RunTickTelemetry rule;
    rule.apply(state);
}

TEST(HC12Manager, DownlinkForward) {
    HC12ManagerTestState state;
    Downlink::Forward rule;
    rule.apply(state);
}

TEST(HC12Manager, DownlinkUartError) {
    HC12ManagerTestState state;
    Downlink::UartError rule;
    rule.apply(state);
}

TEST(HC12Manager, UplinkForward) {
    HC12ManagerTestState state;
    Uplink::Forward rule;
    rule.apply(state);
}

TEST(HC12Manager, UplinkReceiveError) {
    HC12ManagerTestState state;
    Uplink::ReceiveError rule;
    rule.apply(state);
}

TEST(HC12Manager, UplinkReturnBuffer) {
    HC12ManagerTestState state;
    Uplink::ReturnBuffer rule;
    rule.apply(state);
}

TEST(HC12Manager, DrvConnectedNoOp) {
    HC12ManagerTestState state;
    DrvConnected::NoOp rule;
    rule.apply(state);
}

// ---------------------------------------------------------------------------
// Ordered scenario: full happy-path boot from RESET to RUN
// ---------------------------------------------------------------------------
TEST(HC12Manager, OrderedBoot) {
    HC12ManagerTestState state;
    Sm::ResetTick rReset;
    Sm::WaitResetTick rWait;
    Sm::ConfigureNominal rConfigure;
    Sm::RunTickTelemetry rRun;

    rReset.apply(state);
    rWait.apply(state);
    rConfigure.apply(state);
    rRun.apply(state);
}

// ---------------------------------------------------------------------------
// Random scenario: mix all rules, 500 steps. Each rule constructs a fresh
// internal HC12ManagerTestState inside its action body, so the outer `state`
// passed to STest is unused and order-independence holds.
// ---------------------------------------------------------------------------
TEST(HC12Manager, RandomScenario) {
    HC12ManagerTestState state;
    Sm::ResetTick rReset;
    Sm::WaitResetTick rWait;
    Sm::ConfigureNominal rConfigure;
    Sm::RunTickTelemetry rRun;
    Downlink::Forward rDownForward;
    Downlink::UartError rDownError;
    Uplink::Forward rUpForward;
    Uplink::ReceiveError rUpError;
    Uplink::ReturnBuffer rUpReturn;
    DrvConnected::NoOp rDrvConn;

    STest::Rule<HC12ManagerTestState>* rules[] = {
        &rReset, &rWait, &rConfigure, &rRun,
        &rDownForward, &rDownError,
        &rUpForward, &rUpError, &rUpReturn,
        &rDrvConn,
    };
    STest::RandomScenario<HC12ManagerTestState> random("HC12Random", rules,
                                                       FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<HC12ManagerTestState> bounded("HC12Bounded", random, 500);
    const U32 steps = bounded.run(state);
    printf("HC12Manager: ran %u random steps\n", steps);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
