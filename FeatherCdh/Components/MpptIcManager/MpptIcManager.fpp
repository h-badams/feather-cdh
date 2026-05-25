module FeatherCdh {

    @ Flat five-state hardware manager state machine for MpptIcManager
    state machine MpptIcStateMachine {

        initial enter RESET

        @ Rate-group driven signal
        signal tick

        @ Current state action completed successfully
        signal success

        @ Current state action erred
        signal error

        @ BQ25756E INT pin asserted (fault condition); only handled in RUN
        signal fault

        @ Write REG_RST=1 to software-reset all BQ25756E registers
        action doReset

        @ Wait one tick for registers to settle after software reset
        action checkReset

        @ Read PART_INFO (0x3D) to verify BQ25756E device ID
        action doEnable

        @ Write watchdog disable, ADC enable, and charging config registers; read back to verify
        action doConfigure

        @ Read all measurement and status registers; assemble and push BatteryState
        action doRun

        @ Write REG_RST=1 — no hardware reset GPIO; software reset is the equivalent
        state RESET {
            on tick do { doReset }
            on success enter WAIT_RESET
        }

        @ One-tick settling delay after software reset
        state WAIT_RESET {
            on tick do { checkReset }
            on success enter ENABLE
            on error enter RESET
        }

        @ Verify I2C connectivity via PART_INFO read
        state ENABLE {
            on tick do { doEnable }
            on success enter CONFIGURE
            on error enter RESET
        }

        @ Write all BQ25756E configuration registers
        state CONFIGURE {
            on tick do { doConfigure }
            on success enter RUN
            on error enter RESET
        }

        @ Normal operation: read measurements and emit BatteryState each tick
        state RUN {
            on tick do { doRun }
            on error enter RESET
            on fault enter RESET
        }

    }

    @ Layer 2 Active component. Sole owner of BQ25756E MPPT charge controller over I2C.
    active component MpptIcManager {

        @ 1 Hz rate group tick; drives state machine
        async input port schedIn: Svc.Sched

        @ Write BQ25756E registers
        output port busWrite: Drv.I2c

        @ Read BQ25756E registers (write register address then read in one repeated-start transaction)
        output port busWriteRead: Drv.I2cWriteRead

        @ BQ25756E INT pin interrupt — asserted on fault condition; driven by LinuxGpioDriver.gpioInterrupt
        async input port intPin: Svc.Cycle

        @ Battery state output to EPSApplication on each RUN tick
        output port batteryStateOut: FeatherCdh.BatteryStatePort

        @ Register write forwarded from EPSApplication; dropped with WARNING_LO during non-RUN states
        async input port setRegister: FeatherCdh.SetRegisterPort

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event i2cError(
            address: U32,
            status: Drv.I2cStatus
        ) severity warning high \
          format "BQ25756E I2C error on address {} with status {}" \
          throttle 5

        event faultInterrupt(
            faultStatus: U8,
            faultFlag: U8
        ) severity warning high \
          format "BQ25756E INT asserted: faultStatus={} faultFlag={}"

        event registerWritten(
            regAddr: FeatherCdh.BQ25756Reg,
            value: U32
        ) severity activity high \
          format "BQ25756E register {} written with value {}"

        event setRegisterDropped(
            regAddr: FeatherCdh.BQ25756Reg
        ) severity warning low \
          format "setRegister dropped for reg {} — IC not in RUN state"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Battery voltage in mV (VBAT_ADC raw × 2)
        telemetry VBATT_MV: U32

        @ Battery current raw ADC (scaling TBD)
        telemetry IBATT_RAW: U16

        @ Input voltage in mV (VAC_ADC raw × 2)
        telemetry VAC_MV: U32

        @ Input current raw ADC (scaling TBD)
        telemetry IAC_RAW: U16

        @ BQ25756E charging state from CHARGER_STATUS_1 bits [2:0]
        telemetry CHARGING_STATE: FeatherCdh.BQ25756EChargingState

        # ----------------------------------------------------------------------
        # State machine instance
        # ----------------------------------------------------------------------

        state machine instance sm: MpptIcStateMachine

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        telemetry port tlmOut

        event port Log

        text event port LogText

    }

}
