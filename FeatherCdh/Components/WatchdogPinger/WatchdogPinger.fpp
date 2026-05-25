module FeatherCdh {

    @ Layer 2 Passive component. Pulses the TPS3431SDRBR watchdog GPIO on each 10 Hz rate group tick.
    passive component WatchdogPinger {

        @ 10 Hz rate group tick from RateGroup10Hz
        sync input port schedIn: Svc.Sched

        @ GPIO output connected to TPS3431S WDI pin on PDS board
        output port watchdogPing: Drv.GpioWrite

    }

}
