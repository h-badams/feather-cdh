// ======================================================================
// \title  SimplifiedMpptIcManager.cpp
// \author john_tensorflow
// \brief  cpp file for SimplifiedMpptIcManager component implementation class
// ======================================================================

#include "FeatherCdh/Components/SimplifiedMpptIcManager/SimplifiedMpptIcManager.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SimplifiedMpptIcManager::SimplifiedMpptIcManager(const char* const compName)
    : SimplifiedMpptIcManagerComponentBase(compName) {}

SimplifiedMpptIcManager::~SimplifiedMpptIcManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SimplifiedMpptIcManager::intPin_handler(FwIndexType portNum, Os::RawTime& cycleStart) {
    // Note for the MVP we don't check fault registers, we just give off a warning
    this->log_WARNING_HI_faultInterrupt();
}

void SimplifiedMpptIcManager::schedIn_handler(FwIndexType portNum, U32 context) {
    BatteryState state;

    // --- ADC readings (16-bit) ---
    U16 raw16 = 0;

    if (readReg16(BQ25756Reg::VBAT_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_vbatt_mV(static_cast<U32>(raw16) * 2);  // 2 mV/LSB
    }
    if (readReg16(BQ25756Reg::VAC_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_vac_mV(static_cast<U32>(raw16) * 2);    // 2 mV/LSB
    }
    if (readReg16(BQ25756Reg::VFB_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_vfb_mV(static_cast<U32>(raw16) * 2);    // 2 mV/LSB
    }
    if (readReg16(BQ25756Reg::IBAT_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_ibatt_raw(raw16);
    }
    if (readReg16(BQ25756Reg::IAC_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_iac_raw(raw16);
    }
    if (readReg16(BQ25756Reg::TS_ADC, raw16) == Drv::I2cStatus::I2C_OK) {
        state.set_ts_raw(raw16);
    }

    // --- Status registers (8-bit) ---
    U8 raw8 = 0;

    if (readReg8(BQ25756Reg::CHARGER_STATUS_1, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_chargingState(static_cast<BQ25756EChargingState::T>(raw8 & 0x07));
    }
    if (readReg8(BQ25756Reg::CHARGER_STATUS_2, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_chargerStatus2(raw8);
    }
    if (readReg8(BQ25756Reg::CHARGER_STATUS_3, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_chargerStatus3(raw8);
    }
    if (readReg8(BQ25756Reg::FAULT_STATUS, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_faultStatus(raw8);
    }
    if (readReg8(BQ25756Reg::FAULT_FLAG, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_faultFlag(raw8);
    }
    if (readReg8(BQ25756Reg::MPPT_CONT, raw8) == Drv::I2cStatus::I2C_OK) {
        state.set_mppt_enabled((raw8 & 0x01) != 0);
    }

    // Emit telemetry
    this->tlmWrite_VBATT_MV(state.get_vbatt_mV());
    this->tlmWrite_IBATT_RAW(state.get_ibatt_raw());
    this->tlmWrite_VAC_MV(state.get_vac_mV());
    this->tlmWrite_IAC_RAW(state.get_iac_raw());
    this->tlmWrite_CHARGING_STATE(state.get_chargingState());

    // Push BatteryState to EPSApplication
    this->batteryStateOut_out(0, state);
}

void SimplifiedMpptIcManager::setRegister_handler(FwIndexType portNum,
                                                   const FeatherCdh::BQ25756Reg& regAddr,
                                                   U32 value) {
    Drv::I2cStatus status;
    if (isReg16Bit(regAddr)) {
        status = writeReg16(regAddr, static_cast<U16>(value));
    } else {
        status = writeReg8(regAddr, static_cast<U8>(value));
    }

    if (status == Drv::I2cStatus::I2C_OK) {
        this->log_ACTIVITY_HI_registerWritten(regAddr, value);
    }
    // i2cWriteError already emitted inside writeReg8/writeReg16 on failure
}

// ----------------------------------------------------------------------
// Private helpers
// ----------------------------------------------------------------------

bool SimplifiedMpptIcManager::isReg16Bit(BQ25756Reg reg) {
    switch (reg.e) {
        case BQ25756Reg::CHARGE_VOLT_LIM:
        case BQ25756Reg::CHARGE_CURR_LIM:
        case BQ25756Reg::IMP_CURR_DPM_LIM:
        case BQ25756Reg::IMP_VOLT_DPM_LIM:
        case BQ25756Reg::REV_IMP_CURR_LIM:
        case BQ25756Reg::REV_IMP_VOLT_LIM:
        case BQ25756Reg::PRECHARGE_CURR_LIM:
        case BQ25756Reg::TERM_CURR_LIM:
        case BQ25756Reg::VAC_MAX_POW_POINT:
        case BQ25756Reg::IAC_ADC:
        case BQ25756Reg::IBAT_ADC:
        case BQ25756Reg::VAC_ADC:
        case BQ25756Reg::VBAT_ADC:
        case BQ25756Reg::TS_ADC:
        case BQ25756Reg::VFB_ADC:
            return true;
        default:
            return false;
    }
}

Drv::I2cStatus SimplifiedMpptIcManager::writeReg8(BQ25756Reg reg, U8 value) {
    U8 buf[2] = { static_cast<U8>(reg.e), value };
    Fw::Buffer txBuf(buf, sizeof(buf));
    Drv::I2cStatus status = this->busWrite_out(0, BQ25756E_ADDR, txBuf);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_i2cWriteError(static_cast<U8>(reg.e), status);
    }
    return status;
}

Drv::I2cStatus SimplifiedMpptIcManager::writeReg16(BQ25756Reg reg, U16 value) {
    // Big-endian: MSB first
    U8 buf[3] = { static_cast<U8>(reg.e),
                  static_cast<U8>(value >> 8),
                  static_cast<U8>(value & 0xFF) };
    Fw::Buffer txBuf(buf, sizeof(buf));
    Drv::I2cStatus status = this->busWrite_out(0, BQ25756E_ADDR, txBuf);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_i2cWriteError(static_cast<U8>(reg.e), status);
    }
    return status;
}

Drv::I2cStatus SimplifiedMpptIcManager::readReg8(BQ25756Reg reg, U8& value) {
    U8 regByte = static_cast<U8>(reg.e);
    Fw::Buffer txBuf(&regByte, 1);
    Fw::Buffer rxBuf(&value, 1);
    Drv::I2cStatus status = this->busWriteRead_out(0, BQ25756E_ADDR, txBuf, rxBuf);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_i2cReadError(static_cast<U8>(reg.e), status);
    }
    return status;
}

Drv::I2cStatus SimplifiedMpptIcManager::readReg16(BQ25756Reg reg, U16& value) {
    U8 regByte = static_cast<U8>(reg.e);
    U8 rxBytes[2] = {};
    Fw::Buffer txBuf(&regByte, 1);
    Fw::Buffer rxBuf(rxBytes, 2);
    Drv::I2cStatus status = this->busWriteRead_out(0, BQ25756E_ADDR, txBuf, rxBuf);
    if (status != Drv::I2cStatus::I2C_OK) {
        this->log_WARNING_HI_i2cReadError(static_cast<U8>(reg.e), status);
        return status;
    }
    // Big-endian: MSB first
    value = static_cast<U16>((rxBytes[0] << 8) | rxBytes[1]);
    return status;
}

}  // namespace FeatherCdh
