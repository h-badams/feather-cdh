module FeatherCdh {

    @ Layer 2 Active component. Sole owner of BQ25756E MPPT charge controller over I2C.
    @ No init sequence — device runs on power-on register defaults until the operator
    @ sends SET_IC_REGISTER commands from the ground. Notably, ADC_CONT (0x2B) must be
    @ written by the operator before measurement telemetry reflects real values.
    @ Fault interrupts are logged and the INT line is cleared; no re-init is performed.
    active component SimplifiedMpptIcManager {

        # ----------------------------------------------------------------------
        # Ports
        # ----------------------------------------------------------------------

        @ 1 Hz rate group tick; reads all measurement registers and emits BatteryState
        async input port schedIn: Svc.Sched

        @ Write BQ25756E registers
        output port busWrite: Drv.I2c

        @ Read BQ25756E registers (write address then read in one repeated-start transaction)
        output port busWriteRead: Drv.I2cWriteRead

        @ BQ25756E INT pin interrupt — driven by LinuxGpioDriver.gpioInterrupt.
        @ INT is a 256 µs active-low pulse; it self-deasserts and requires no register read to release.
        @ Handler reads FAULT_STATUS and FAULT_FLAG to capture the source, then emits faultInterrupt.
        async input port intPin: Svc.Cycle

        @ Battery state output to EPSApplication on each tick
        output port batteryStateOut: FeatherCdh.BatteryStatePort

        @ Register write forwarded from EPSApplication; always honored
        async input port setRegister: FeatherCdh.SetRegisterPort

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ I2C read failed for a measurement or status register during a schedIn tick
        event i2cReadError(
            regAddr: U8,
            status: Drv.I2cStatus
        ) severity warning high \
          format "BQ25756E I2C read error at register 0x{x} with status {}" \
          throttle 5

        @ I2C write failed during a setRegister call
        event i2cWriteError(
            regAddr: U8,
            status: Drv.I2cStatus
        ) severity warning high \
          format "BQ25756E I2C write error at register 0x{x} with status {}" \
          throttle 5

        @ BQ25756E INT pin asserted
        event faultInterrupt severity warning high \
          format "BQ25756E fault interrupt"

        @ setRegister call completed successfully
        event registerWritten(
            regAddr: FeatherCdh.BQ25756Reg,
            value: U32
        ) severity activity high \
          format "BQ25756E register {} written with value {}"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Battery voltage in mV (VBAT_ADC raw × 2); zero until ADC enabled via ground command
        telemetry VBATT_MV: U32

        @ Battery current raw ADC (scaling TBD); zero until ADC enabled via ground command
        telemetry IBATT_RAW: U16

        @ Input voltage in mV (VAC_ADC raw × 2); zero until ADC enabled via ground command
        telemetry VAC_MV: U32

        @ Input current raw ADC (scaling TBD); zero until ADC enabled via ground command
        telemetry IAC_RAW: U16

        @ BQ25756E charging state from CHARGER_STATUS_1 bits [2:0]
        telemetry CHARGING_STATE: FeatherCdh.BQ25756EChargingState

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        telemetry port tlmOut

        event port Log

        text event port LogText

    }

}
