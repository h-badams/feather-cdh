#include <gtest/gtest.h>
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"
#include "STest/Random/Random.hpp"
#include "Rules.hpp"

using namespace FeatherCdh;

// ---------------------------------------------------------------------------
// Direct test: single nominal ping
// ---------------------------------------------------------------------------
TEST(WatchdogPinger, PingNominal) {
    WatchdogPingerTestState state;
    WatchdogPingerRules::Ping::Nominal rule;
    rule.apply(state);
}

// ---------------------------------------------------------------------------
// Random scenario: 200 pings in random order (only one rule, so all are Ping)
// ---------------------------------------------------------------------------
TEST(WatchdogPinger, RandomPings) {
    WatchdogPingerTestState state;
    WatchdogPingerRules::Ping::Nominal ruleNominal;

    STest::Rule<WatchdogPingerTestState>* rules[] = {&ruleNominal};
    STest::RandomScenario<WatchdogPingerTestState> random("WatchdogRandom", rules,
                                                          FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<WatchdogPingerTestState> bounded("WatchdogBounded", random, 200);
    const U32 steps = bounded.run(state);
    printf("WatchdogPinger: ran %u random steps\n", steps);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
