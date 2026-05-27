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
using namespace FeatherCdh::CommsApplicationRules;

// ---------------------------------------------------------------------------
// Individual GTest cases
// ---------------------------------------------------------------------------

TEST(CommsApplication, SmSafeOnBoot) {
    CommsApplicationTestState state;
    Sm::SafeOnBoot rule;
    rule.apply(state);
}

TEST(CommsApplication, SmSafeToNormal) {
    CommsApplicationTestState state;
    Sm::SafeToNormal rule;
    rule.apply(state);
}

TEST(CommsApplication, SmNormalToSafe) {
    CommsApplicationTestState state;
    Sm::NormalToSafe rule;
    rule.apply(state);
}

TEST(CommsApplication, SmSafeIgnoresSafe) {
    CommsApplicationTestState state;
    Sm::SafeIgnoresSafe rule;
    rule.apply(state);
}

TEST(CommsApplication, SmNormalIgnoresNormal) {
    CommsApplicationTestState state;
    Sm::NormalIgnoresNormal rule;
    rule.apply(state);
}

TEST(CommsApplication, DrainNormalEveryTick) {
    CommsApplicationTestState state;
    Drain::NormalEveryTick rule;
    rule.apply(state);
}

TEST(CommsApplication, DrainSafeEveryDivisor) {
    CommsApplicationTestState state;
    Drain::SafeEveryDivisor rule;
    rule.apply(state);
}

TEST(CommsApplication, DrainSafeSuppressesBetween) {
    CommsApplicationTestState state;
    Drain::SafeSuppressesBetween rule;
    rule.apply(state);
}

TEST(CommsApplication, DrainModeSwitchResetsCounter) {
    CommsApplicationTestState state;
    Drain::ModeSwitchResetsCounter rule;
    rule.apply(state);
}

TEST(CommsApplication, TlmCommsModeEachTick) {
    CommsApplicationTestState state;
    Tlm::CommsModeEachTick rule;
    rule.apply(state);
}

TEST(CommsApplication, PingEcho) {
    CommsApplicationTestState state;
    Ping::Echo rule;
    rule.apply(state);
}

// ---------------------------------------------------------------------------
// Random scenario: each rule constructs a fresh internal state in its action,
// so the outer state passed to STest is unused and ordering is independent.
// ---------------------------------------------------------------------------
TEST(CommsApplication, RandomScenario) {
    CommsApplicationTestState state;
    Sm::SafeOnBoot rSafeOnBoot;
    Sm::SafeToNormal rSafeToNormal;
    Sm::NormalToSafe rNormalToSafe;
    Sm::SafeIgnoresSafe rSafeIgnoresSafe;
    Sm::NormalIgnoresNormal rNormalIgnoresNormal;
    Drain::NormalEveryTick rNormalEveryTick;
    Drain::SafeEveryDivisor rSafeEveryDivisor;
    Drain::SafeSuppressesBetween rSafeSuppressesBetween;
    Drain::ModeSwitchResetsCounter rModeSwitchResets;
    Tlm::CommsModeEachTick rTlmTick;
    Ping::Echo rPing;

    STest::Rule<CommsApplicationTestState>* rules[] = {
        &rSafeOnBoot,           &rSafeToNormal,          &rNormalToSafe,
        &rSafeIgnoresSafe,      &rNormalIgnoresNormal,   &rNormalEveryTick,
        &rSafeEveryDivisor,     &rSafeSuppressesBetween, &rModeSwitchResets,
        &rTlmTick,              &rPing,
    };
    STest::RandomScenario<CommsApplicationTestState> random("CommsAppRandom", rules,
                                                            FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<CommsApplicationTestState> bounded("CommsAppBounded", random, 50);
    const U32 steps = bounded.run(state);
    printf("CommsApplication: ran %u random steps\n", steps);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
