#pragma once
#include "SatStateMachineTester.hpp"

namespace FeatherCdh {

class SatStateMachineTestState : public SatStateMachineTester {
  public:
    // -----------------------------------------------------------------------
    // Tick group — preconditioned on SM state
    // -----------------------------------------------------------------------

    // SAFE, vbatt above threshold → CURRENT_MODE=Safe telemetry, no events
    bool precondition__Tick__InSafeNominal() const;
    void action__Tick__InSafeNominal();

    // SAFE, vbatt below threshold → criticalBattery event, SM stays SAFE
    // (SAFE state has no handler for criticalVoltage per FPP; signal is silently dropped)
    bool precondition__Tick__InSafeCritical() const;
    void action__Tick__InSafeCritical();

    // STANDBY, vbatt above threshold → CURRENT_MODE=Standby telemetry, no events
    bool precondition__Tick__InStandbyNominal() const;
    void action__Tick__InStandbyNominal();

    // STANDBY, vbatt below threshold → criticalBattery + criticalVoltage signal
    // → STANDBY→SAFE transition → enteredSafe + commsModeOut(Safe)
    bool precondition__Tick__InStandbyCritical() const;
    void action__Tick__InStandbyCritical();

    // -----------------------------------------------------------------------
    // Cmd group
    // -----------------------------------------------------------------------

    // SAFE_MODE from STANDBY → STANDBY→SAFE, enteredSafe, commsModeOut(Safe), CMD OK
    bool precondition__Cmd__SafeMode() const;
    void action__Cmd__SafeMode();

    // SAFE_EXIT from SAFE → SAFE→STANDBY, enteredStandby, commsModeOut(Normal), CMD OK
    bool precondition__Cmd__SafeExit() const;
    void action__Cmd__SafeExit();

    // SAFE_EXIT from STANDBY → safeExitRejected event, CMD EXECUTION_ERROR, SM unchanged
    bool precondition__Cmd__SafeExitRejected() const;
    void action__Cmd__SafeExitRejected();

    // -----------------------------------------------------------------------
    // Ping group
    // -----------------------------------------------------------------------

    // pingIn → pingOut carries the same key (always applicable)
    bool precondition__Ping__Nominal() const;
    void action__Ping__Nominal();
};

}  // namespace FeatherCdh
