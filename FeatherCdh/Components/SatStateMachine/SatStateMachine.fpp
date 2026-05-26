module FeatherCdh {

    @ Flat two-state mode machine for SatStateMachine.
    @ Handles mode transitions only; telemetry and WARNING_HI events are emitted
    @ directly in the schedIn C++ handler, not via SM actions.
    state machine SatModeStateMachine {

        initial enter SAFE

        @ vbatt fell below CRITICAL_THRESHOLD; sent by schedIn handler
        signal criticalVoltage

        @ SAFE_MODE ground command received
        signal safeModeCmd

        @ SAFE_EXIT ground command received; C++ only sends this when SM is in SAFE
        signal safeExitCmd

        @ Send commsModeOut(Comms.CommsMode.Safe) and emit mode transition event
        action sendSafeMode

        @ Send commsModeOut(Comms.CommsMode.Normal) and emit mode transition event
        action sendStandbyMode

        @ Satellite boots in Safe mode
        state SAFE {
            entry do { sendSafeMode }
            on safeExitCmd enter STANDBY
        }

        @ Normal operations; transitions to SAFE on critical battery or ground command
        state STANDBY {
            entry do { sendStandbyMode }
            on criticalVoltage enter SAFE
            on safeModeCmd enter SAFE
        }

    }

    @ Layer 4 Active component. Sole authority on satellite operational mode.
    @ Evaluates battery voltage each 1 Hz tick and responds to SAFE_MODE / SAFE_EXIT
    @ ground commands; sends typed mode commands to Layer 3 application components.
    active component SatStateMachine {

        # ----------------------------------------------------------------------
        # Ports
        # ----------------------------------------------------------------------

        @ 1 Hz rate group tick; evaluates power state and emits mode telemetry
        async input port schedIn: Svc.Sched

        @ Mode command to CommsApplication; fired on every Safe <-> Standby transition
        output port commsModeOut: FeatherCdh.CommsModePort

        @ Sync get into EPSApplication; returns cached PowerState without recomputation
        output port powerStateGet: FeatherCdh.PowerStateGetPort

        @ Health ping input from Svc.Health
        async input port pingIn: Svc.Ping

        @ Health ping response back to Svc.Health
        output port pingOut: Svc.Ping

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------

        @ Force transition to Safe mode from any state.
        async command SAFE_MODE

        @ Transition from Safe to Standby. Rejected with EXECUTION_ERROR if not in Safe.
        async command SAFE_EXIT

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ Battery voltage threshold (mV) for autonomous Safe mode transition
        param CRITICAL_THRESHOLD: U32 default 0

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event enteredSafe \
          severity activity high \
          format "SatStateMachine entered Safe mode"

        event enteredStandby \
          severity activity high \
          format "SatStateMachine entered Standby mode"

        event criticalBattery(
            vbatt_mV: U32
        ) severity warning high \
          format "Battery voltage {} mV is below CRITICAL_THRESHOLD; holding or entering Safe mode"

        event safeExitRejected \
          severity warning low \
          format "SAFE_EXIT rejected: satellite is not in Safe mode"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Current satellite operational mode; emitted every 1 Hz tick
        telemetry CURRENT_MODE: Sat.Mode

        # ----------------------------------------------------------------------
        # State machine instance
        # ----------------------------------------------------------------------

        state machine instance sm: SatModeStateMachine

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        telemetry port tlmOut

        event port Log

        text event port LogText

        command recv port CmdDisp

        command reg port CmdReg

        command resp port CmdStatus

        param get port prmGet

        param set port prmSet

    }

}
