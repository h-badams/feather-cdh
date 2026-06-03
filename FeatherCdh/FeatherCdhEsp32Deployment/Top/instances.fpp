module FeatherCdh {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (2 for this deployment)
  #   SS  = Subtopology digits (00 for main topology, 01-05 for subtopologies)
  #   CC  = Component digits (00, 01, 02, etc.)
  #   xxx = Reserved for internal component items (events, commands, telemetry)
  #

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    # Reduced from 64 KiB — ESP32-S3 has 512 KiB SRAM shared across all tasks
    constant STACK_SIZE = 8 * 1024
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  # 10 Hz rate group: CommsApplication, UART stream driver poll, ComCcsds aggregator flush
  instance rateGroup10Hz: Svc.ActiveRateGroup base id 0x20001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  # 1 Hz rate group: all application/manager components + infrastructure
  instance rateGroup1Hz: Svc.ActiveRateGroup base id 0x20002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  # Layer 4 — Mission Orchestration
  instance satStateMachine: FeatherCdh.SatStateMachine base id 0x20015000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 30

  # Layer 3 — Application Components
  instance epsApplication: FeatherCdh.EPSApplication base id 0x20016000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 29

  instance commsApplication: FeatherCdh.CommsApplication base id 0x20017000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 28

  # Layer 2 — Hardware Managers
  instance mpptIcManager: FeatherCdh.SimplifiedMpptIcManager base id 0x20018000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 27

  # HC12Manager is queued (no dedicated thread); driven by sync schedIn on the 1 Hz group
  instance hc12Manager: FeatherCdh.HC12Manager base id 0x20019000 \
    queue size Default.QUEUE_SIZE

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  # Arduino time service (replaces PosixTime / ChronoTime)
  instance arduinoTime: Arduino.ArduinoTime base id 0x20010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x20011000

  # Hardware timer ISR driver (replaces LinuxTimer); ticks at 50 Hz
  instance hardwareRateDriver: Arduino.HardwareRateDriver base id 0x20013000

  # Layer 1 — Arduino Hardware Drivers

  # HC-12 UART — wraps Serial1; polled via schedIn at 10 Hz
  instance streamDriver: Arduino.StreamDriver base id 0x20014000

  # BQ25756E I2C bus (BMS board) — wraps Wire
  instance i2cDriverBms: Arduino.I2cDriver base id 0x2001A000

  # HC-12 SET pin — GPIO output, default HIGH (transparent mode)
  instance gpioDriverSetPin: Arduino.GpioDriver base id 0x2001B000

}

# ----------------------------------------------------------------------
# TlmPacketizer override — replaces the default TlmChan from
# CdhCoreTlmConfig.fpp.  CdhCoreTlmConfig.fpp must be excluded via
# register_fprime_config CONFIGURATION_OVERRIDES in CMakeLists.txt.
# ----------------------------------------------------------------------
module CdhCore {

  instance tlmSend: Svc.TlmPacketizer base id CdhCoreConfig.BASE_ID + 0x06000 \
    queue size CdhCoreConfig.QueueSizes.tlmSend \
    stack size CdhCoreConfig.StackSizes.tlmSend \
    priority CdhCoreConfig.Priorities.tlmSend \
  {
    phase Fpp.ToCpp.Phases.configComponents """
    CdhCore::tlmSend.setPacketList(
        FeatherCdh::FeatherCdhEsp32Deployment_FeatherCdhEsp32DeploymentPacketsTlmPackets::packetList,
        FeatherCdh::FeatherCdhEsp32Deployment_FeatherCdhEsp32DeploymentPacketsTlmPackets::omittedChannels,
        1
    );
    """
  }

}
