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
using namespace FeatherCdh::EPSApplicationRules;

// ---------------------------------------------------------------------------
// Individual GTest cases (one rule per case, deterministic)
// ---------------------------------------------------------------------------

TEST(EPSApplication, BatteryStateReceive) {
    EPSApplicationTestState state;
    BatteryState::Receive rule;
    rule.apply(state);
}

TEST(EPSApplication, TickNominal) {
    EPSApplicationTestState state;
    Tick::Nominal rule;
    rule.apply(state);
}

TEST(EPSApplication, TickLowBattery) {
    EPSApplicationTestState state;
    Tick::LowBattery rule;
    rule.apply(state);
}

TEST(EPSApplication, TickCriticalBattery) {
    EPSApplicationTestState state;
    Tick::CriticalBattery rule;
    rule.apply(state);
}

TEST(EPSApplication, TickStaleBattery) {
    EPSApplicationTestState state;
    Tick::StaleBattery rule;
    rule.apply(state);
}

TEST(EPSApplication, SetIcRegisterForward) {
    EPSApplicationTestState state;
    SetIcRegister::Forward rule;
    rule.apply(state);
}

TEST(EPSApplication, PowerStateGetCheck) {
    EPSApplicationTestState state;
    PowerStateGet::Check rule;
    rule.apply(state);
}

// ---------------------------------------------------------------------------
// Ordered scenario: full receive-tick-get pipeline
// ---------------------------------------------------------------------------
TEST(EPSApplication, OrderedPipeline) {
    EPSApplicationTestState state;
    BatteryState::Receive rReceive;
    Tick::Nominal rTick;
    PowerStateGet::Check rGet;

    rReceive.apply(state);
    rTick.apply(state);
    rGet.apply(state);
}

// ---------------------------------------------------------------------------
// Random scenario: mix of all rules, 500 steps
// ---------------------------------------------------------------------------
TEST(EPSApplication, RandomScenario) {
    EPSApplicationTestState state;
    BatteryState::Receive rReceive;
    Tick::Nominal rNominal;
    Tick::LowBattery rLow;
    Tick::CriticalBattery rCritical;
    SetIcRegister::Forward rSetReg;
    PowerStateGet::Check rGet;

    STest::Rule<EPSApplicationTestState>* rules[] = {
        &rReceive, &rNominal, &rLow, &rCritical, &rSetReg, &rGet
    };
    STest::RandomScenario<EPSApplicationTestState> random("EPSRandom", rules,
                                                          FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<EPSApplicationTestState> bounded("EPSBounded", random, 500);
    const U32 steps = bounded.run(state);
    printf("EPSApplication: ran %u random steps\n", steps);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
