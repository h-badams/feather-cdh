module FeatherCdh {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (2 for desktop deployment)
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

  # 10 Hz rate group (fires at 10 Hz from the 100 ms base tick; divisor 1)
  instance rateGroup10Hz: Svc.ActiveRateGroup base id 0x20001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  # 1 Hz rate group (fires at 1 Hz from the 100 ms base tick; divisor 10)
  instance rateGroup1Hz: Svc.ActiveRateGroup base id 0x20002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance cmdSeq: Svc.CmdSequencer base id 0x20004000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 40

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
  # Queued component instances
  # ----------------------------------------------------------------------


  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance posixTime: Svc.PosixTime base id 0x20010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x20011000

  instance systemResources: Svc.SystemResources base id 0x20012000

  # 100 ms base-rate interval timer; startTimer() called via startRateGroups() in Main.cpp
  instance linuxTimer: Svc.LinuxTimer base id 0x20013000 \
  {
    phase Fpp.ToCpp.Phases.stopTasks """
    linuxTimer.quit();
    """
  }

  # Layer 1 — Drivers
  # Note: with FPRIME_USE_STUBBED_DRIVERS=ON (fprime-desktop preset), LinuxI2cDriver
  # and LinuxGpioDriver compile their stub implementations — no real hardware access.

  # TcpClient replaces LinuxUartDriver; GDS acts as TCP server, deployment connects as client.
  # configure() and start() are called in Topology.cpp using state.hostname / state.port.
  instance comDriver: Drv.TcpClient base id 0x20014000

  # BQ25756E I2C bus — stub open() returns true; handlers return I2C_OK with zero-filled buffers
  instance i2cDriverBms: Drv.LinuxI2cDriver base id 0x2001A000 \
  {
    phase Fpp.ToCpp.Phases.configComponents """
    i2cDriverBms.open("/dev/i2c-1");
    // Stub returns true; no FW_ASSERT since real hardware is absent on desktop
    """
  }

  # HC-12 SET pin — stub open() returns NOT_SUPPORTED; HC12Manager handles GPIO errors gracefully
  instance gpioDriverSetPin: Drv.LinuxGpioDriver base id 0x2001B000

  # BQ25756E INT — stub poll loop does nothing; no fault interrupts will fire on desktop
  instance gpioDriverInt: Drv.LinuxGpioDriver base id 0x2001C000

}

# ----------------------------------------------------------------------
# TlmPacketizer override — replaces default TlmChan from CdhCoreTlmConfig.fpp
# ----------------------------------------------------------------------
module CdhCore {

  instance tlmSend: Svc.TlmPacketizer base id CdhCoreConfig.BASE_ID + 0x06000 \
    queue size CdhCoreConfig.QueueSizes.tlmSend \
    stack size CdhCoreConfig.StackSizes.tlmSend \
    priority CdhCoreConfig.Priorities.tlmSend \
  {
    phase Fpp.ToCpp.Phases.configComponents """
    CdhCore::tlmSend.setPacketList(
        FeatherCdh::FeatherCdhDesktopDeployment_FeatherCdhDesktopDeploymentPacketsTlmPackets::packetList,
        FeatherCdh::FeatherCdhDesktopDeployment_FeatherCdhDesktopDeploymentPacketsTlmPackets::omittedChannels,
        1
    );
    """
  }

}
