module FeatherCdh {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    rateGroup10Hz = 0
    rateGroup1Hz  = 1
  }

  topology FeatherCdhEsp32Deployment {

  # ----------------------------------------------------------------------
  # Subtopology imports
  # ----------------------------------------------------------------------
    import CdhCore.Subtopology
    import ComCcsds.FramingSubtopology
    import FileHandling.Subtopology

  # ----------------------------------------------------------------------
  # Instances used in the topology
  # ----------------------------------------------------------------------

    # Scheduling infrastructure
    instance hardwareRateDriver
    instance arduinoTime
    instance rateGroupDriver
    instance rateGroup10Hz
    instance rateGroup1Hz

    # Layer 4 — Mission Orchestration
    instance satStateMachine

    # Layer 3 — Application Components
    instance epsApplication
    instance commsApplication

    # Layer 2 — Hardware Managers
    instance mpptIcManager
    instance hc12Manager

    # Layer 1 — Drivers
    instance i2cDriverBms
    instance streamDriver
    instance gpioDriverSetPin

  # ----------------------------------------------------------------------
  # Pattern graph specifiers
  # ----------------------------------------------------------------------

    command   connections instance CdhCore.cmdDisp
    event     connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    health    connections instance CdhCore.$health
    param     connections instance FileHandling.prmDb
    time      connections instance arduinoTime

  # ----------------------------------------------------------------------
  # Telemetry packets (TlmPacketizer)
  # ----------------------------------------------------------------------

    include "FeatherCdhEsp32DeploymentPackets.fppi"

  # ----------------------------------------------------------------------
  # Direct graph specifiers
  # ----------------------------------------------------------------------

    connections RateGroups {
      # 50 Hz hardware timer ISR → RateGroupDriver
      hardwareRateDriver.CycleOut[0] -> rateGroupDriver.CycleIn

      # 10 Hz group (divisor 5): CommsApplication, ComCcsds aggregator flush, UART poll
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup10Hz] -> rateGroup10Hz.CycleIn
      rateGroup10Hz.RateGroupMemberOut[0] -> commsApplication.schedIn
      rateGroup10Hz.RateGroupMemberOut[1] -> ComCcsds.aggregator.timeout
      rateGroup10Hz.RateGroupMemberOut[2] -> streamDriver.schedIn

      # 1 Hz group (divisor 50): application, managers, infrastructure.
      # MpptIcManager fires first (slot 0) so it pushes a fresh BatteryState into
      # EPSApplication's queue before EPSApplication's schedIn (slot 1) processes it.
      # SatStateMachine fires after EPSApplication (slot 2) so its powerStateGet sync
      # call returns the current-cycle PowerState.
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1Hz] -> rateGroup1Hz.CycleIn
      rateGroup1Hz.RateGroupMemberOut[0]  -> mpptIcManager.schedIn
      rateGroup1Hz.RateGroupMemberOut[1]  -> epsApplication.schedIn
      rateGroup1Hz.RateGroupMemberOut[2]  -> satStateMachine.schedIn
      rateGroup1Hz.RateGroupMemberOut[3]  -> hc12Manager.schedIn
      rateGroup1Hz.RateGroupMemberOut[4]  -> CdhCore.$health.Run
      rateGroup1Hz.RateGroupMemberOut[5]  -> CdhCore.tlmSend.Run
      rateGroup1Hz.RateGroupMemberOut[6]  -> CdhCore.cmdDisp.run
      rateGroup1Hz.RateGroupMemberOut[7]  -> FileHandling.fileDownlink.Run
      rateGroup1Hz.RateGroupMemberOut[8]  -> FileHandling.fileManager.schedIn
      rateGroup1Hz.RateGroupMemberOut[9]  -> ComCcsds.commsBufferManager.schedIn
    }

    connections EPS {
      # MpptIcManager ↔ BQ25756E over I2C (BMS board)
      mpptIcManager.busWrite     -> i2cDriverBms.write
      mpptIcManager.busWriteRead -> i2cDriverBms.writeRead

      # MpptIcManager → EPSApplication: BatteryState each 1 Hz tick
      mpptIcManager.batteryStateOut -> epsApplication.batteryStateIn

      # EPSApplication → MpptIcManager: SET_IC_REGISTER forwarding
      epsApplication.setRegister -> mpptIcManager.setRegister

      # SatStateMachine → EPSApplication: sync get for PowerState
      satStateMachine.powerStateGet -> epsApplication.powerStateGet
    }

    connections Comms {
      # SatStateMachine → CommsApplication: mode commands
      satStateMachine.commsModeOut -> commsApplication.commsModeIn

      # CommsApplication is the sole driver of ComQueue drain (not a rate group)
      commsApplication.comQueueRun -> ComCcsds.comQueue.run

      # 5 Svc.Com connections: hc12Manager replaces ComStub as the Com Adapter
      # Downlink: framer → hc12Manager → wire
      ComCcsds.framer.dataOut                 -> hc12Manager.dataIn
      hc12Manager.dataReturnOut               -> ComCcsds.framer.dataReturnIn
      hc12Manager.comStatusOut                -> ComCcsds.framer.comStatusIn
      # Uplink: wire → hc12Manager → frameAccumulator
      hc12Manager.dataOut                     -> ComCcsds.frameAccumulator.dataIn
      ComCcsds.frameAccumulator.dataReturnOut -> hc12Manager.dataReturnIn

      # hc12Manager ↔ streamDriver (downward ByteStreamDriverClient interface)
      hc12Manager.drvSendOut          -> streamDriver.$send
      streamDriver.ready              -> hc12Manager.drvConnected
      streamDriver.$recv              -> hc12Manager.drvReceiveIn
      hc12Manager.drvReceiveReturnOut -> streamDriver.recvReturnIn

      # HC-12 SET pin (LOW = AT command mode, HIGH = transparent mode)
      hc12Manager.setPin -> gpioDriverSetPin.gpioWrite

      # streamDriver borrows receive buffers from the ComCcsds buffer pool
      streamDriver.allocate   -> ComCcsds.commsBufferManager.bufferGetCallee
      streamDriver.deallocate -> ComCcsds.commsBufferManager.bufferSendIn
    }

    connections ComCcsds_CdhCore {
      # Events and telemetry into the ComQueue downlink pipeline
      CdhCore.events.PktSend  -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]

      # Uplink: routed commands from ground → CmdDispatcher
      ComCcsds.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus     -> ComCcsds.fprimeRouter.cmdResponseIn
    }

    connections ComCcsds_FileHandling {
      # File downlink: FileDownlink → ComQueue buffer queue
      FileHandling.fileDownlink.bufferSendOut -> ComCcsds.comQueue.bufferQueueIn[ComCcsds.Ports_ComBufferQueue.FILE]
      ComCcsds.comQueue.bufferReturnOut[ComCcsds.Ports_ComBufferQueue.FILE] -> FileHandling.fileDownlink.bufferReturn

      # File uplink: ComCcsds router → FileUplink
      ComCcsds.fprimeRouter.fileOut         -> FileHandling.fileUplink.bufferSendIn
      FileHandling.fileUplink.bufferSendOut -> ComCcsds.fprimeRouter.fileBufferReturnIn
    }

    connections FeatherCdhEsp32Deployment {

    }

  }

}
