module FeatherCdh {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    rateGroup10Hz = 0
    rateGroup1Hz  = 1
  }

  topology FeatherCdhDesktopDeployment {

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
    instance linuxTimer
    instance posixTime
    instance rateGroupDriver
    instance rateGroup10Hz
    instance rateGroup1Hz

    # CDH infrastructure
    instance cmdSeq
    instance systemResources

    # Layer 4 — Mission Orchestration
    instance satStateMachine

    # Layer 3 — Application Components
    instance epsApplication
    instance commsApplication

    # Layer 2 — Hardware Managers
    instance mpptIcManager
    instance hc12Manager

    # Layer 1 — Drivers (stubbed on desktop via FPRIME_USE_STUBBED_DRIVERS)
    instance i2cDriverBms
    instance comDriver
    instance gpioDriverSetPin
    instance gpioDriverInt

  # ----------------------------------------------------------------------
  # Pattern graph specifiers
  # ----------------------------------------------------------------------

    command   connections instance CdhCore.cmdDisp
    event     connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    health    connections instance CdhCore.$health
    param     connections instance FileHandling.prmDb
    time      connections instance posixTime

  # ----------------------------------------------------------------------
  # Telemetry packets (TlmPacketizer)
  # ----------------------------------------------------------------------

    include "FeatherCdhDesktopDeploymentPackets.fppi"

  # ----------------------------------------------------------------------
  # Direct graph specifiers
  # ----------------------------------------------------------------------

    connections RateGroups {
      # 100 ms base tick → RateGroupDriver
      linuxTimer.CycleOut -> rateGroupDriver.CycleIn

      # 10 Hz group (divisor 1): CommsApplication + ComCcsds aggregator flush
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup10Hz] -> rateGroup10Hz.CycleIn
      rateGroup10Hz.RateGroupMemberOut[0] -> commsApplication.schedIn
      rateGroup10Hz.RateGroupMemberOut[1] -> ComCcsds.aggregator.timeout

      # 1 Hz group (divisor 10): application, managers, infrastructure.
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
      rateGroup1Hz.RateGroupMemberOut[9]  -> systemResources.run
      rateGroup1Hz.RateGroupMemberOut[10] -> cmdSeq.schedIn
      rateGroup1Hz.RateGroupMemberOut[11] -> ComCcsds.commsBufferManager.schedIn
    }

    connections EPS {
      # mpptIcManager ↔ BQ25756E over I2C (stub returns zeros on desktop)
      mpptIcManager.busWrite     -> i2cDriverBms.write
      mpptIcManager.busWriteRead -> i2cDriverBms.writeRead

      # BQ25756E INT pin → mpptIcManager (stub never fires on desktop)
      gpioDriverInt.gpioInterrupt -> mpptIcManager.intPin

      # mpptIcManager → EPSApplication: BatteryState each 1 Hz tick
      mpptIcManager.batteryStateOut -> epsApplication.batteryStateIn

      # EPSApplication → mpptIcManager: SET_IC_REGISTER forwarding
      epsApplication.setRegister -> mpptIcManager.setRegister

      # SatStateMachine → EPSApplication: sync get for PowerState
      satStateMachine.powerStateGet -> epsApplication.powerStateGet
    }

    connections Comms {
      # SatStateMachine → CommsApplication: mode commands
      satStateMachine.commsModeOut -> commsApplication.commsModeIn

      # CommsApplication is the sole driver of ComQueue drain
      commsApplication.comQueueRun -> ComCcsds.comQueue.run

      # 5 Svc.Com connections: hc12Manager replaces ComStub as the Com Adapter
      # Downlink: framer → hc12Manager → wire
      ComCcsds.framer.dataOut                 -> hc12Manager.dataIn
      hc12Manager.dataReturnOut               -> ComCcsds.framer.dataReturnIn
      hc12Manager.comStatusOut                -> ComCcsds.framer.comStatusIn
      # Uplink: wire → hc12Manager → frameAccumulator
      hc12Manager.dataOut                     -> ComCcsds.frameAccumulator.dataIn
      ComCcsds.frameAccumulator.dataReturnOut -> hc12Manager.dataReturnIn

      # hc12Manager ↔ comDriver (downward ByteStreamDriverClient interface)
      hc12Manager.drvSendOut          -> comDriver.$send
      comDriver.ready                 -> hc12Manager.drvConnected
      comDriver.$recv                 -> hc12Manager.drvReceiveIn
      hc12Manager.drvReceiveReturnOut -> comDriver.recvReturnIn

      # HC-12 SET pin (stub on desktop — GPIO errors handled gracefully)
      hc12Manager.setPin -> gpioDriverSetPin.gpioWrite

      # comDriver borrows receive buffers from the ComCcsds buffer pool
      comDriver.allocate   -> ComCcsds.commsBufferManager.bufferGetCallee
      comDriver.deallocate -> ComCcsds.commsBufferManager.bufferSendIn
    }

    connections ComCcsds_CdhCore {
      CdhCore.events.PktSend  -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]

      ComCcsds.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus     -> ComCcsds.fprimeRouter.cmdResponseIn
    }

    connections ComCcsds_FileHandling {
      FileHandling.fileDownlink.bufferSendOut -> ComCcsds.comQueue.bufferQueueIn[ComCcsds.Ports_ComBufferQueue.FILE]
      ComCcsds.comQueue.bufferReturnOut[ComCcsds.Ports_ComBufferQueue.FILE] -> FileHandling.fileDownlink.bufferReturn

      ComCcsds.fprimeRouter.fileOut        -> FileHandling.fileUplink.bufferSendIn
      FileHandling.fileUplink.bufferSendOut -> ComCcsds.fprimeRouter.fileBufferReturnIn
    }

    connections CdhCore_cmdSeq {
      cmdSeq.comCmdOut             -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> cmdSeq.cmdResponseIn
    }

    connections FeatherCdhDesktopDeployment {

    }

  }

}
