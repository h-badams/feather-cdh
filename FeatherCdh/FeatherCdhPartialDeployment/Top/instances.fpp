module FeatherCdh {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (1 for this deployment)
  #   SS  = Subtopology digits (00 for main topology, 01-05 for subtopologies)
  #   CC  = Component digits (00, 01, 02, etc.)
  #   xxx = Reserved for internal component items (events, commands, telemetry)
  #

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  # 10 Hz rate group: CommsApplication, ComCcsds aggregator flush
  instance rateGroup10Hz: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  # 1 Hz rate group: all application/manager components + infrastructure
  instance rateGroup1Hz: Svc.ActiveRateGroup base id 0x10002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance cmdSeq: Svc.CmdSequencer base id 0x10004000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 40

  # Layer 4 — Mission Orchestration
  instance satStateMachine: FeatherCdh.SatStateMachine base id 0x10015000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 30

  # Layer 3 — Application Components
  instance epsApplication: FeatherCdh.EPSApplication base id 0x10016000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 29

  instance commsApplication: FeatherCdh.CommsApplication base id 0x10017000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 28

  # Layer 2 — Hardware Managers
  instance mpptIcManager: FeatherCdh.SimplifiedMpptIcManager base id 0x10018000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 27

  # HC12Manager is queued (no dedicated thread); driven by sync schedIn on the 1 Hz group
  instance hc12Manager: FeatherCdh.HC12Manager base id 0x10019000 \
    queue size Default.QUEUE_SIZE

  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------


  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance posixTime: Svc.PosixTime base id 0x10010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x10011000

  instance systemResources: Svc.SystemResources base id 0x10012000

  # 50 Hz base-rate interval timer; startTimer() is called via startRateGroups()
  # in Main.cpp, which blocks until stopRateGroups() is called from the signal handler.
  instance linuxTimer: Svc.LinuxTimer base id 0x10013000

  # Layer 1 — Hardware Drivers (PDS-dependent components omitted)

  # HC-12 UART (9600 8N1, no flow control)
  # open() and start() are called in Topology.cpp setupTopology() using state.uartDevice/baudRate
  instance uartDriver: Drv.LinuxUartDriver base id 0x10014000

  # BQ25756E I2C bus (BMS board)
  instance i2cDriverBms: Drv.LinuxI2cDriver base id 0x1001A000

  # HC-12 SET pin — GPIO output, default HIGH (transparent mode)
  instance gpioDriverSetPin: Drv.LinuxGpioDriver base id 0x1001B000

  # BQ25756E INT — GPIO input, falling-edge interrupt; driver starts its own poll thread
  instance gpioDriverInt: Drv.LinuxGpioDriver base id 0x1001C000

}

# ----------------------------------------------------------------------
# TlmPacketizer override — replaces the default TlmChan from
# CdhCoreTlmConfig.fpp.  CdhCoreTlmConfig.fpp must be removed from
# the CMakeLists.txt SOURCE_FILES list to avoid a duplicate-instance
# error at autocoder time.
# ----------------------------------------------------------------------
module CdhCore {

  instance tlmSend: Svc.TlmPacketizer base id CdhCoreConfig.BASE_ID + 0x06000 \
    queue size CdhCoreConfig.QueueSizes.tlmSend \
    stack size CdhCoreConfig.StackSizes.tlmSend \
    priority CdhCoreConfig.Priorities.tlmSend \
  {
    phase Fpp.ToCpp.Phases.configComponents """
    CdhCore::tlmSend.setPacketList(
        FeatherCdh::FeatherCdhPartialDeployment_FeatherCdhPartialDeploymentPacketsTlmPackets::packetList,
        FeatherCdh::FeatherCdhPartialDeployment_FeatherCdhPartialDeploymentPacketsTlmPackets::omittedChannels,
        1
    );
    """
  }

}
