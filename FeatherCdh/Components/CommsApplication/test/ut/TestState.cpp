#include "TestState.hpp"

namespace FeatherCdh {

// Each rule constructs a fresh CommsApplicationTestState for isolation. Protected
// assert_*() methods are reachable on `fresh` because protected access is permitted
// between instances of the same class.

// ---------------------------------------------------------------------------
// Rule: Sm::SafeOnBoot — after init, component is in SAFE; one tick yields SAFE
// telemetry and no comQueueRun (counter increments to 1, < divisor).
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Sm__SafeOnBoot() const {
    return true;
}

void CommsApplicationTestState::action__Sm__SafeOnBoot() {
    CommsApplicationTestState fresh;
    fresh.tickOnce();

    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assertTlm_COMMS_MODE_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_COMMS_MODE(__FILE__, __LINE__, 0, Comms::CommsMode::Safe);
}

// ---------------------------------------------------------------------------
// Rule: Sm::SafeToNormal
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Sm__SafeToNormal() const {
    return true;
}

void CommsApplicationTestState::action__Sm__SafeToNormal() {
    CommsApplicationTestState fresh;
    fresh.sendModeCommand(Comms::CommsMode::Normal);

    fresh.assertEvents_modeTransition_size(__FILE__, __LINE__, 1);
    fresh.assertEvents_modeTransition(__FILE__, __LINE__, 0, Comms::CommsMode::Normal);
}

// ---------------------------------------------------------------------------
// Rule: Sm::NormalToSafe
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Sm__NormalToSafe() const {
    return true;
}

void CommsApplicationTestState::action__Sm__NormalToSafe() {
    CommsApplicationTestState fresh;
    fresh.driveToNormal();
    fresh.sendModeCommand(Comms::CommsMode::Safe);

    fresh.assertEvents_modeTransition_size(__FILE__, __LINE__, 1);
    fresh.assertEvents_modeTransition(__FILE__, __LINE__, 0, Comms::CommsMode::Safe);
}

// ---------------------------------------------------------------------------
// Rule: Sm::SafeIgnoresSafe — switchMode(Safe) while already SAFE: no transition.
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Sm__SafeIgnoresSafe() const {
    return true;
}

void CommsApplicationTestState::action__Sm__SafeIgnoresSafe() {
    CommsApplicationTestState fresh;
    fresh.sendModeCommand(Comms::CommsMode::Safe);

    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Sm::NormalIgnoresNormal
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Sm__NormalIgnoresNormal() const {
    return true;
}

void CommsApplicationTestState::action__Sm__NormalIgnoresNormal() {
    CommsApplicationTestState fresh;
    fresh.driveToNormal();
    fresh.sendModeCommand(Comms::CommsMode::Normal);

    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Drain::NormalEveryTick — in NORMAL, comQueueRun fires every tick.
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Drain__NormalEveryTick() const {
    return true;
}

void CommsApplicationTestState::action__Drain__NormalEveryTick() {
    CommsApplicationTestState fresh;
    fresh.driveToNormal();

    const U32 N = 5;
    for (U32 i = 0; i < N; ++i) {
        fresh.tickOnce();
    }

    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, N);
}

// ---------------------------------------------------------------------------
// Rule: Drain::SafeEveryDivisor — in SAFE, 10 ticks => exactly 1 comQueueRun.
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Drain__SafeEveryDivisor() const {
    return true;
}

void CommsApplicationTestState::action__Drain__SafeEveryDivisor() {
    CommsApplicationTestState fresh;

    for (U32 i = 0; i < CommsApplicationTester::SAFE_DRAIN_DIVISOR; ++i) {
        fresh.tickOnce();
    }

    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 1);
}

// ---------------------------------------------------------------------------
// Rule: Drain::SafeSuppressesBetween — divisor-1 ticks yields 0 drain.
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Drain__SafeSuppressesBetween() const {
    return true;
}

void CommsApplicationTestState::action__Drain__SafeSuppressesBetween() {
    CommsApplicationTestState fresh;

    for (U32 i = 0; i < CommsApplicationTester::SAFE_DRAIN_DIVISOR - 1; ++i) {
        fresh.tickOnce();
    }

    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Drain::ModeSwitchResetsCounter — re-entering SAFE resets the counter
// (no SM history; entry action zeros m_safeDrainCounter).
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Drain__ModeSwitchResetsCounter() const {
    return true;
}

void CommsApplicationTestState::action__Drain__ModeSwitchResetsCounter() {
    CommsApplicationTestState fresh;
    const U32 divisor = CommsApplicationTester::SAFE_DRAIN_DIVISOR;

    // Tick divisor-1 times in SAFE (counter = divisor-1, no drain yet).
    for (U32 i = 0; i < divisor - 1; ++i) {
        fresh.tickOnce();
    }
    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);

    // Round-trip mode to reset counter.
    fresh.sendModeCommand(Comms::CommsMode::Normal);
    fresh.sendModeCommand(Comms::CommsMode::Safe);
    fresh.clearHistory();

    // divisor-1 ticks again: must NOT fire — counter restarted from 0.
    for (U32 i = 0; i < divisor - 1; ++i) {
        fresh.tickOnce();
    }
    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 0);

    // One more tick reaches divisor and fires.
    fresh.tickOnce();
    fresh.assert_from_comQueueRun_size(__FILE__, __LINE__, 1);
}

// ---------------------------------------------------------------------------
// Rule: Tlm::CommsModeEachTick — COMMS_MODE telemetry written each tick;
// value tracks the current SM mode.
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Tlm__CommsModeEachTick() const {
    return true;
}

void CommsApplicationTestState::action__Tlm__CommsModeEachTick() {
    CommsApplicationTestState fresh;

    fresh.tickOnce();
    fresh.assertTlm_COMMS_MODE_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_COMMS_MODE(__FILE__, __LINE__, 0, Comms::CommsMode::Safe);

    fresh.sendModeCommand(Comms::CommsMode::Normal);
    fresh.clearHistory();

    fresh.tickOnce();
    fresh.assertTlm_COMMS_MODE_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_COMMS_MODE(__FILE__, __LINE__, 0, Comms::CommsMode::Normal);
}

// ---------------------------------------------------------------------------
// Rule: Ping::Echo
// ---------------------------------------------------------------------------

bool CommsApplicationTestState::precondition__Ping__Echo() const {
    return true;
}

void CommsApplicationTestState::action__Ping__Echo() {
    CommsApplicationTestState fresh;
    const U32 key = 0xDEADBEEF;

    fresh.invoke_to_pingIn(0, key);
    fresh.drainAll();

    fresh.assert_from_pingOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(key, fresh.fromPortHistory_pingOut->at(0).key);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assertTlm_size(__FILE__, __LINE__, 0);
}

}  // namespace FeatherCdh
