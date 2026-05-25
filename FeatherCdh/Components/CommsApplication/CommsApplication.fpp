module FeatherCdh {

    @ Flat two-state state machine for CommsApplication
    state machine CommsAppStateMachine {

        initial enter SAFE

        @ Rate-group driven signal; from schedIn handler
        signal tick

        @ Mode command from SatStateMachine; carries new Comms.Mode value
        signal switchMode: Comms.Mode

        @ True if the switchMode signal carries Comms.Mode.Normal
        guard isModeNormal: Comms.Mode

        @ True if the switchMode signal carries Comms.Mode.Safe
        guard isModeSafe: Comms.Mode

        @ Emit mode-transition event for Safe entry
        action emitSafeMode

        @ Emit mode-transition event for Normal entry
        action emitNormalMode

        @ Fire comQueueRun every SAFE_DRAIN_DIVISOR ticks
        action doSafeDrain

        @ Fire comQueueRun every tick
        action doNormalDrain

        @ Reduced-rate ComQueue drain; mode transition event on entry
        state SAFE {
            entry do { emitSafeMode }
            on tick do { doSafeDrain }
            on switchMode if isModeNormal enter NORMAL
        }

        @ Full-rate ComQueue drain; mode transition event on entry
        state NORMAL {
            entry do { emitNormalMode }
            on tick do { doNormalDrain }
            on switchMode if isModeSafe enter SAFE
        }

    }

    @ Layer 3 Active component. Controls ComCcsds ComQueue drain rate based on mode.
    @ Boots in Safe mode (reduced-rate drain) until SatStateMachine commands Normal.
    active component CommsApplication {

        # ----------------------------------------------------------------------
        # Ports
        # ----------------------------------------------------------------------

        @ 10 Hz rate group tick; drives state machine
        async input port schedIn: Svc.Sched

        @ Mode command from SatStateMachine
        async input port commsModeIn: FeatherCdh.CommsModePort

        @ Fires ComCcsds ComQueue drain; connected to comQueue.run
        output port comQueueRun: Svc.Sched

        @ Health ping input from Svc.Health
        async input port pingIn: Svc.Ping

        @ Health ping response back to Svc.Health
        output port pingOut: Svc.Ping

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ Number of 10 Hz ticks between ComQueue drains in Safe mode.
        @ Default of 10 gives 1 Hz drain rate in Safe vs 10 Hz in Normal.
        param SAFE_DRAIN_DIVISOR: U32 default 10

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event modeTransition(
            mode: Comms.Mode
        ) severity activity high \
          format "CommsApplication transitioned to {} mode"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Current comms mode; emitted every schedIn tick
        telemetry COMMS_MODE: Comms.Mode

        # ----------------------------------------------------------------------
        # State machine instance
        # ----------------------------------------------------------------------

        state machine instance sm: CommsAppStateMachine

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
