telemetry packets FeatherCdhDesktopDeploymentPackets {

  packet CDH id 1 group 1 {
    CdhCore.cmdDisp.CommandsDispatched
    CdhCore.cmdDisp.CommandsDropped
    #CdhCore.tlmSend.SendLevel # uncomment when using TlmPacketizer

    FileHandling.fileUplink.FilesReceived
    FileHandling.fileUplink.PacketsReceived
    FileHandling.fileDownlink.FilesSent
    FileHandling.fileDownlink.PacketsSent
    FileHandling.fileManager.CommandsExecuted

    FeatherCdh.cmdSeq.CS_LoadCommands
    FeatherCdh.cmdSeq.CS_CancelCommands
    FeatherCdh.cmdSeq.CS_CommandsExecuted
    FeatherCdh.cmdSeq.CS_SequencesCompleted
    ComCcsds.comQueue.comQueueDepth
    ComCcsds.comQueue.buffQueueDepth
    ComCcsds.commsBufferManager.TotalBuffs
    ComCcsds.commsBufferManager.CurrBuffs
    ComCcsds.commsBufferManager.HiBuffs

    FeatherCdh.rateGroup10Hz.RgMaxTime
    FeatherCdh.rateGroup1Hz.RgMaxTime
  }

  packet CDHErrors id 2 group 1 {
    CdhCore.$health.PingLateWarnings

    FileHandling.fileUplink.Warnings
    FileHandling.fileDownlink.Warnings
    FileHandling.fileManager.Errors

    FeatherCdh.cmdSeq.CS_Errors
    ComCcsds.commsBufferManager.NoBuffs
    ComCcsds.commsBufferManager.EmptyBuffs

    FeatherCdh.rateGroup10Hz.RgCycleSlips
    FeatherCdh.rateGroup1Hz.RgCycleSlips
  }

  packet SystemRes1 id 5 group 2 {
    FeatherCdh.systemResources.MEMORY_TOTAL
    FeatherCdh.systemResources.MEMORY_USED
    FeatherCdh.systemResources.NON_VOLATILE_TOTAL
    FeatherCdh.systemResources.NON_VOLATILE_FREE
  }

  packet SystemRes3 id 6 group 2 {
    FeatherCdh.systemResources.CPU
    FeatherCdh.systemResources.CPU_00
    FeatherCdh.systemResources.CPU_01
    FeatherCdh.systemResources.CPU_02
    FeatherCdh.systemResources.CPU_03
    FeatherCdh.systemResources.CPU_04
    FeatherCdh.systemResources.CPU_05
    FeatherCdh.systemResources.CPU_06
    FeatherCdh.systemResources.CPU_07
    FeatherCdh.systemResources.CPU_08
    FeatherCdh.systemResources.CPU_09
    FeatherCdh.systemResources.CPU_10
    FeatherCdh.systemResources.CPU_11
    FeatherCdh.systemResources.CPU_12
    FeatherCdh.systemResources.CPU_13
    FeatherCdh.systemResources.CPU_14
    FeatherCdh.systemResources.CPU_15
  }

  packet EPS id 10 group 3 {
    FeatherCdh.mpptIcManager.VBATT_MV
    FeatherCdh.mpptIcManager.VAC_MV
    FeatherCdh.mpptIcManager.IBATT_RAW
    FeatherCdh.mpptIcManager.IAC_RAW
    FeatherCdh.mpptIcManager.CHARGING_STATE
    FeatherCdh.satStateMachine.CURRENT_MODE
  }

  packet Comms id 11 group 3 {
    FeatherCdh.hc12Manager.BYTES_SENT
    FeatherCdh.hc12Manager.BYTES_RECV
    FeatherCdh.hc12Manager.UART_ERRORS
    FeatherCdh.commsApplication.COMMS_MODE
  }

  packet Version1 id 22 group 2 {
    CdhCore.version.FrameworkVersion
    CdhCore.version.ProjectVersion
  }

  packet Version_Library1 id 23 group 2 {
    CdhCore.version.LibraryVersion01
    CdhCore.version.LibraryVersion02
  }

  packet Version_Library2 id 24 group 2 {
    CdhCore.version.LibraryVersion03
    CdhCore.version.LibraryVersion04
  }

  packet Version_Library3 id 25 group 2 {
    CdhCore.version.LibraryVersion05
    CdhCore.version.LibraryVersion06
  }

  packet Version_Library4 id 26 group 2 {
    CdhCore.version.LibraryVersion07
    CdhCore.version.LibraryVersion08
  }

  packet Version_Library5 id 27 group 2 {
    CdhCore.version.LibraryVersion09
    CdhCore.version.LibraryVersion10
  }

  packet Version_Custom1 id 28 group 2 {
    CdhCore.version.CustomVersion01
  }

  packet Version_Custom2 id 29 group 2 {
    CdhCore.version.CustomVersion02
  }

  packet Version_Custom3 id 30 group 2 {
    CdhCore.version.CustomVersion03
  }

  packet Version_Custom4 id 31 group 2 {
    CdhCore.version.CustomVersion04
  }

  packet Version_Custom5 id 32 group 2 {
    CdhCore.version.CustomVersion05
  }

  packet Version_Custom6 id 33 group 2 {
    CdhCore.version.CustomVersion06
  }

  packet Version_Custom7 id 34 group 2 {
    CdhCore.version.CustomVersion07
  }

  packet Version_Custom8 id 35 group 2 {
    CdhCore.version.CustomVersion08
  }

  packet Version_Custom9 id 36 group 2 {
    CdhCore.version.CustomVersion09
  }

  packet Version_Custom10 id 37 group 2 {
    CdhCore.version.CustomVersion10
  }

} omit {
  CdhCore.cmdDisp.CommandErrors
  CdhCore.tlmSend.GroupConfigs
  CdhCore.tlmSend.SectionEnabled
}
