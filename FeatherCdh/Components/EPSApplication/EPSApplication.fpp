module FeatherCdh {

    @ Layer 3 Active component. Monitors battery health, caches PowerState for SatStateMachine,
    @ and forwards SET_IC_REGISTER commands to MpptIcManager. No state machine — runs
    @ continuously regardless of satellite mode.
    active component EPSApplication {

        # ----------------------------------------------------------------------
        # Ports
        # ----------------------------------------------------------------------

        @ 1 Hz rate group tick; assembles and caches PowerState
        async input port schedIn: Svc.Sched

        @ Battery state pushed by MpptIcManager each tick; cached for next schedIn processing
        async input port batteryStateIn: FeatherCdh.BatteryStatePort

        @ Sync get called by SatStateMachine on its own thread; returns cached PowerState
        guarded input port powerStateGet: FeatherCdh.PowerStateGetPort

        @ Register write forwarded unconditionally to MpptIcManager
        output port setRegister: FeatherCdh.SetRegisterPort

        @ Health ping input from Svc.Health
        async input port pingIn: Svc.Ping

        @ Health ping response back to Svc.Health
        output port pingOut: Svc.Ping

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------

        @ Write a BQ25756E register via MpptIcManager. Forwarded unconditionally.
        async command SET_IC_REGISTER(
            regAddr: FeatherCdh.BQ25756Reg,
            value: U32
        )

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ Battery voltage threshold (mV) for LOW_BATTERY warning event
        param POWER_THRESHOLD: U32 default 0

        @ Battery voltage threshold (mV) for CRITICAL_BATTERY warning event
        param CRITICAL_THRESHOLD: U32 default 0

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event lowBattery(
            vbatt_mV: U32
        ) severity warning high \
          format "Battery voltage {} mV is below POWER_THRESHOLD"

        event criticalBattery(
            vbatt_mV: U32
        ) severity warning high \
          format "Battery voltage {} mV is below CRITICAL_THRESHOLD"

        event icRegisterForwarded(
            regAddr: FeatherCdh.BQ25756Reg,
            value: U32
        ) severity activity high \
          format "SET_IC_REGISTER forwarded: reg={} value={}"

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        event port Log

        text event port LogText

        command recv port CmdDisp

        command reg port CmdReg

        command resp port CmdStatus

        param get port prmGet

        param set port prmSet

    }

}
