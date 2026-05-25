module FeatherCdh {

    @ Flat four-state hardware manager state machine for HC12Manager
    state machine HC12StateMachine {

        initial enter RESET

        @ Rate-group driven signal
        signal tick

        @ Current state action completed successfully
        signal success

        @ Current state action erred
        signal error

        @ Assert SET pin LOW and delay 40 ms for AT command mode entry
        action doReset

        @ One-tick settling delay after SET LOW before sending AT commands
        action doWaitReset

        @ Send AT config commands; assert SET HIGH; delay 80 ms; call ready port
        action doConfigure

        @ Emit telemetry; byte passthrough handled directly by port handlers
        action doRun

        @ Assert SET LOW and delay 40 ms to enter AT command mode
        state RESET {
            on tick do { doReset }
            on success enter WAIT_RESET
        }

        @ One-tick settling delay after SET LOW before AT configuration
        state WAIT_RESET {
            on tick do { doWaitReset }
            on success enter CONFIGURE
            on error enter RESET
        }

        @ Send AT configuration commands and verify each response
        state CONFIGURE {
            on tick do { doConfigure }
            on success enter RUN
            on error enter RESET
        }

        @ Normal operation: byte passthrough between ComCcsds and LinuxUartDriver
        state RUN {
            on tick do { doRun }
            on error enter RESET
        }

    }

    @ Layer 2 Active component. Manages HC-12 433 MHz UART radio module.
    @ Implements Drv::ByteStreamDriver toward ComCcsds and Drv::ByteStreamDriverClient toward LinuxUartDriver.
    queued component HC12Manager {

        # ----------------------------------------------------------------------
        # ByteStreamDriver interface — toward ComCcsds (comStub)
        # ----------------------------------------------------------------------

        import ByteStreamDriver

        # ----------------------------------------------------------------------
        # ByteStreamDriverClient interface — toward LinuxUartDriver
        # ----------------------------------------------------------------------

        import ByteStreamDriverClient

        # ----------------------------------------------------------------------
        # Other ports
        # ----------------------------------------------------------------------

        @ 1 Hz rate group tick; drives state machine
        sync input port schedIn: Svc.Sched

        @ HC-12 SET pin: LOW = AT command mode, HIGH = transparent mode
        output port setPin: Drv.GpioWrite

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        event atCommandFailed \
          severity warning high \
          format "HC-12 AT configuration command failed; resetting" \
          throttle 3

        event uartSendError(
            status: Drv.ByteStreamStatus
        ) severity warning high \
          format "HC-12 UART send error with status {}; resetting" \
          throttle 5

        event runEntry \
          severity activity high \
          format "HC12Manager entered RUN state; radio ready"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Total bytes sent to LinuxUartDriver
        telemetry BYTES_SENT: U32

        @ Total bytes received from LinuxUartDriver
        telemetry BYTES_RECV: U32

        @ Cumulative UART send error count
        telemetry UART_ERRORS: U32

        # ----------------------------------------------------------------------
        # State machine instance
        # ----------------------------------------------------------------------

        state machine instance sm: HC12StateMachine

        # ----------------------------------------------------------------------
        # Standard ports
        # ----------------------------------------------------------------------

        time get port timeCaller

        telemetry port tlmOut

        event port Log

        text event port LogText

    }

}
