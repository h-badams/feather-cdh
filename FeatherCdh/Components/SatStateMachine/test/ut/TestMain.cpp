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
using namespace FeatherCdh::SatStateMachineRules;

// ---------------------------------------------------------------------------
// Individual GTest cases (one rule per case, deterministic)
// ---------------------------------------------------------------------------

TEST(SatStateMachine, TickInSafeNominal) {
    SatStateMachineTestState state;
    Tick::InSafeNominal rule;
    rule.apply(state);
}

TEST(SatStateMachine, TickInSafeCritical) {
    SatStateMachineTestState state;
    Tick::InSafeCritical rule;
    rule.apply(state);
}

// Boot into SAFE then transition to STANDBY first, then test
TEST(SatStateMachine, TickInStandbyNominal) {
    SatStateMachineTestState state;
    state.transitionToStandby();
    Tick::InStandbyNominal rule;
    rule.apply(state);
}

TEST(SatStateMachine, TickInStandbyCritical) {
    SatStateMachineTestState state;
    state.transitionToStandby();
    Tick::InStandbyCritical rule;
    rule.apply(state);
}

TEST(SatStateMachine, CmdSafeMode) {
    SatStateMachineTestState state;
    state.transitionToStandby();
    Cmd::SafeMode rule;
    rule.apply(state);
}

TEST(SatStateMachine, CmdSafeExit) {
    SatStateMachineTestState state;
    Cmd::SafeExit rule;
    rule.apply(state);
}

TEST(SatStateMachine, CmdSafeExitRejected) {
    SatStateMachineTestState state;
    state.transitionToStandby();
    Cmd::SafeExitRejected rule;
    rule.apply(state);
}

TEST(SatStateMachine, PingNominal) {
    SatStateMachineTestState state;
    Ping::Nominal rule;
    rule.apply(state);
}

// ---------------------------------------------------------------------------
// Ordered scenario: walks through major mode paths in sequence
//   Boot(SAFE) → tick nominal → tick critical-in-SAFE (stays) →
//   SAFE_EXIT → tick nominal in STANDBY → SAFE_EXIT rejected →
//   SAFE_MODE → tick nominal back in SAFE → ping
// ---------------------------------------------------------------------------

TEST(SatStateMachine, OrderedLifecycle) {
    SatStateMachineTestState state;

    Tick::InSafeNominal    tickSafeNom;
    Tick::InSafeCritical   tickSafeCrit;
    Cmd::SafeExit          safeExit;
    Tick::InStandbyNominal tickStandbyNom;
    Cmd::SafeExitRejected  safeExitRejected;
    Cmd::SafeMode          safeMode;
    Ping::Nominal          ping;

    tickSafeNom.apply(state);      // SAFE, vbatt ok
    tickSafeCrit.apply(state);     // SAFE, vbatt critical → stays SAFE
    safeExit.apply(state);         // SAFE → STANDBY
    tickStandbyNom.apply(state);   // STANDBY, vbatt ok
    safeExitRejected.apply(state); // SAFE_EXIT rejected (in STANDBY)
    safeMode.apply(state);         // STANDBY → SAFE
    tickSafeNom.apply(state);      // SAFE, vbatt ok again
    ping.apply(state);             // health ping round-trip
}

// ---------------------------------------------------------------------------
// Random scenario: all rules, preconditions guard state-dependent transitions
// ---------------------------------------------------------------------------

TEST(SatStateMachine, RandomScenario) {
    SatStateMachineTestState state;

    Tick::InSafeNominal    tickSafeNom;
    Tick::InSafeCritical   tickSafeCrit;
    Tick::InStandbyNominal tickStandbyNom;
    Tick::InStandbyCritical tickStandbyCrit;
    Cmd::SafeMode          safeMode;
    Cmd::SafeExit          safeExit;
    Cmd::SafeExitRejected  safeExitRejected;
    Ping::Nominal          ping;

    STest::Rule<SatStateMachineTestState>* rules[] = {
        &tickSafeNom,
        &tickSafeCrit,
        &tickStandbyNom,
        &tickStandbyCrit,
        &safeMode,
        &safeExit,
        &safeExitRejected,
        &ping,
    };

    STest::RandomScenario<SatStateMachineTestState> random(
        "SSMRandom", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<SatStateMachineTestState> bounded(
        "SSMBounded", random, 500);

    const U32 steps = bounded.run(state);
    printf("SatStateMachine: ran %u random steps\n", steps);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
