module Comms {

    @ Comms subsystem mode - received by CommsApplication from SatStateMachine
    enum Mode : U8 {
        Safe   = 0  @< ComQueue drain suppressed; no autonomous downlink
        Normal = 1  @< ComQueue drained each 10 Hz tick; autonomous downlink active
    }

}

module FeatherCdh {

    # ------------------------------------------------------------------
    # BQ25756E enumerations
    # ------------------------------------------------------------------

    @ BQ25756E charging state — CHARGER_STATUS_1 (0x21) bits [2:0]
    enum BQ25756EChargingState : U8 {
        NOT_CHARGING    = 0x00  @< No charging in progress
        TRICKLE_CHARGE  = 0x01  @< VFB below VBAT_SHORT threshold
        PRE_CHARGE      = 0x02  @< VBAT_SHORT <= VFB < VBAT_LOWV
        FAST_CHARGE_CC  = 0x03  @< Constant-current fast charge
        TAPER_CHARGE_CV = 0x04  @< Constant-voltage taper charge
        RESERVED        = 0x05
        TOP_OFF_TIMER   = 0x06  @< Top-off timer active
        CHARGE_DONE     = 0x07  @< Charge termination done
    }

    @ BQ25756E register addresses — used in SET_IC_REGISTER command and MpptIcManager
    enum BQ25756Reg : U8 {
        CHARGE_VOLT_LIM        = 0x00  @< Charge voltage limit (16-bit)
        CHARGE_CURR_LIM        = 0x02  @< Charge current limit; bits [10:2], 50 mA/LSB (16-bit)
        IMP_CURR_DPM_LIM       = 0x06  @< Input current DPM limit (16-bit)
        IMP_VOLT_DPM_LIM       = 0x08  @< Input voltage DPM limit (16-bit)
        REV_IMP_CURR_LIM       = 0x0A  @< Reverse mode input current limit (16-bit)
        REV_IMP_VOLT_LIM       = 0x0C  @< Reverse mode input voltage limit (16-bit)
        PRECHARGE_CURR_LIM     = 0x10  @< Precharge current limit (16-bit)
        TERM_CURR_LIM          = 0x12  @< Termination current limit (16-bit)
        VAC_MAX_POW_POINT      = 0x1F  @< VAC max power point detected (16-bit)
        IAC_ADC                = 0x2D  @< Input current ADC reading (16-bit)
        IBAT_ADC               = 0x2F  @< Battery current ADC reading (16-bit)
        VAC_ADC                = 0x31  @< Input voltage ADC reading; 2 mV/LSB (16-bit)
        VBAT_ADC               = 0x33  @< Battery voltage ADC reading; 2 mV/LSB (16-bit)
        TS_ADC                 = 0x37  @< Temperature sensor ADC reading (16-bit)
        VFB_ADC                = 0x39  @< Feedback voltage ADC reading; 2 mV/LSB (16-bit)
        PRECHARGE_TERM_CONT    = 0x14  @< Precharge and termination control (8-bit)
        TIME_CONT              = 0x15  @< Timer control; disable internal watchdog here (8-bit)
        THR_STG_CHARGE_CONT    = 0x16  @< Three-stage charge control (8-bit)
        CHARGER_CONT           = 0x17  @< Charger control; bit 5 = WD_RST (watchdog kick only) (8-bit)
        PIN_CONT_REG           = 0x18  @< Pin control (8-bit)
        POW_PATH_REV_CONT      = 0x19  @< Power path and reverse mode control; bit 7 = REG_RST (8-bit)
        MPPT_CONT              = 0x1A  @< MPPT control; bit 0 = EN_MPPT; bits [2:1] = FULL_SWEEP_TMR (8-bit)
        CHARGE_THRESH_CONT     = 0x1B  @< Charging threshold control (8-bit)
        CHARGE_REGION_CONT     = 0x1C  @< Charging region behavior control (8-bit)
        REV_THRESH_CONT        = 0x1D  @< Reverse mode threshold control (8-bit)
        REV_UNDERVOLT_CONT     = 0x1E  @< Reverse undervoltage control (8-bit)
        CHARGER_STATUS_1       = 0x21  @< Charger status 1 (read-only); bits [2:0] = charging state (8-bit)
        CHARGER_STATUS_2       = 0x22  @< Charger status 2 (read-only) (8-bit)
        CHARGER_STATUS_3       = 0x23  @< Charger status 3 (read-only) (8-bit)
        FAULT_STATUS           = 0x24  @< Fault status (read-only) (8-bit)
        CHARGER_FLAG_1         = 0x25  @< Charger flag 1 (8-bit)
        CHARGER_FLAG_2         = 0x26  @< Charger flag 2 (8-bit)
        FAULT_FLAG             = 0x27  @< Fault flag; cleared on read (8-bit)
        CHARGER_MASK_1         = 0x28  @< INT mask 1 (8-bit)
        CHARGER_MASK_2         = 0x29  @< INT mask 2 (8-bit)
        FAULT_MASK             = 0x2A  @< Fault INT mask (8-bit)
        ADC_CONT               = 0x2B  @< ADC control; enable and mode select (8-bit)
        ADC_CHANNEL_CONT       = 0x2C  @< ADC channel enable; ignored when EN_MPPT=1 (8-bit)
        GATE_DRV_STRG_CONT     = 0x3B  @< Gate driver strength control (8-bit)
        GATE_DRV_DEATTIME_CONT = 0x3C  @< Gate driver dead time control (8-bit)
        PART_INFO              = 0x3D  @< Part information / device ID (8-bit)
        REV_BATT_DISCHARGE_CURR = 0x62 @< Reverse mode battery discharge current (8-bit)
    }

    # ------------------------------------------------------------------
    # Shared data structs
    # ------------------------------------------------------------------

    @ Full BQ25756E measurement and status snapshot — published by MpptIcManager each tick
    struct BatteryState {
        vbatt_mV:      U32,  @< Battery voltage in mV (VBAT_ADC * 2)
        ibatt_raw:     U16,  @< Battery current raw ADC (scaling TBD)
        vac_mV:        U32,  @< Input voltage in mV (VAC_ADC * 2)
        iac_raw:       U16,  @< Input current raw ADC (scaling TBD)
        vfb_mV:        U32,  @< Feedback voltage in mV (VFB_ADC * 2)
        ts_raw:        U16,  @< Temperature sensor raw ADC (scaling TBD)
        chargingState: BQ25756EChargingState,
        chargerStatus2: U8,
        chargerStatus3: U8,
        faultStatus:   U8,
        faultFlag:     U8,
        mppt_enabled:  bool  @< EN_MPPT bit from MPPT_CONT (0x1A bit 0)
    } default {
        chargingState = BQ25756EChargingState.NOT_CHARGING
        mppt_enabled = false
    }

    @ Power state cache — assembled by EPSApplication, read by SatStateMachine via powerStateGet
    struct PowerState {
        vbatt_mV:      U32,
        chargingState: BQ25756EChargingState,
        faultStatus:   U8
    } default {
        chargingState = BQ25756EChargingState.NOT_CHARGING
    }

    # ------------------------------------------------------------------
    # Custom ports
    # ------------------------------------------------------------------

    @ Battery state from MpptIcManager to EPSApplication (async, queued)
    port BatteryStatePort(ref batteryState: BatteryState)

    @ Register write from EPSApplication to MpptIcManager (async, queued)
    port SetRegisterPort(regAddr: BQ25756Reg, value: U32)

    @ Sync get port: SatStateMachine calls into EPSApplication to retrieve cached PowerState
    port PowerStateGetPort() -> PowerState

    @ Mode command from SatStateMachine to CommsApplication
    port CommsModePort(mode: Comms.Mode)

}
