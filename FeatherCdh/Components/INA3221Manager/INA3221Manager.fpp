module FeatherCdh {

    @ Flat five-state hardware manager state machine for INA3221Manager
    state machine INA3221StateMachine {

        initial enter RESET

        @ Rate-group driven signal
        signal tick

        @ Current state action completed successfully
        signal success

        @ Current state action erred
        signal error

        @ Assert SET — no-op for INA3221 (no hardware reset GPIO)
        action doReset

        @ Wait one tick for IC settling
        action checkReset

        @ Read Manufacturer ID register to verify I2C connectivity
        action doEnable

        @ Write configuration register (averaging, conversion time, channel enable)
        action doConfigure

        @ Read all six channel registers and emit telemetry
        action doRun

        @ Assert hardware reset — no GPIO on INA3221; immediately signal success
        state RESET {
            on tick do { doReset }
            on success enter WAIT_RESET
        }

        @ One-tick settling delay
        state WAIT_RESET {
            on tick do { checkReset }
            on success enter ENABLE
            on error enter RESET
        }

        @ Verify I2C connectivity via Manufacturer ID read
        state ENABLE {
            on tick do { doEnable }
            on success enter CONFIGURE
            on error enter RESET
        }

        @ Write INA3221 configuration register
        state CONFIGURE {
            on tick do { doConfigure }
            on success enter RUN
            on error enter RESET
        }

        @ Normal operation: read rails and emit telemetry each tick
        state RUN {
            on tick do { doRun }
            on error enter RESET
        }

    }

    @ Layer 2 Queued component. Reads INA3221 triple-channel current/voltage monitor on PDS board.
    @ PDS-dependent — included in FeatherCdhFullDeployment only.
    queued component INA3221Manager {

        @ 1 Hz rate group tick; drives state machine
        sync input port schedIn: Svc.Sched

        @ Write INA3221 registers
        output port busWrite: Drv.I2c

        @ Read INA3221 registers (write pointer then read in one repeated-start transaction)
        output port busWriteRead: Drv.I2cWriteRead

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event i2cError(
            address: U32,
            status: Drv.I2cStatus
        ) severity warning high \
          format "INA3221 I2C error on address {} with status {}" \
          throttle 5

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ 12 V rail bus voltage (mV)
        telemetry CH1_VOLTAGE: U32

        @ 12 V rail current (mA)
        telemetry CH1_CURRENT: F32

        @ 5 V rail bus voltage (mV)
        telemetry CH2_VOLTAGE: U32

        @ 5 V rail current (mA)
        telemetry CH2_CURRENT: F32

        @ 3.3 V rail bus voltage (mV)
        telemetry CH3_VOLTAGE: U32

        @ 3.3 V rail current (mA)
        telemetry CH3_CURRENT: F32

        # ----------------------------------------------------------------------
        # State machine instance
        # ----------------------------------------------------------------------

        state machine instance sm: INA3221StateMachine

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        telemetry port tlmOut

        event port Log

        text event port LogText

    }

}
