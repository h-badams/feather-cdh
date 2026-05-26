// ======================================================================
// \title  EPSApplication.cpp
// \author john_tensorflow
// \brief  cpp file for EPSApplication component implementation class
// ======================================================================

#include "FeatherCdh/Components/EPSApplication/EPSApplication.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

EPSApplication ::EPSApplication(const char* const compName) : EPSApplicationComponentBase(compName) {}

EPSApplication ::~EPSApplication() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void EPSApplication ::batteryStateIn_handler(FwIndexType portNum, FeatherCdh::BatteryState& batteryState) {
    m_batteryState = batteryState;
}

void EPSApplication ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

FeatherCdh::PowerState EPSApplication ::powerStateGet_handler(FwIndexType portNum) {
    return m_powerState;
}

void EPSApplication ::schedIn_handler(FwIndexType portNum, U32 context) {
    Fw::ParamValid valid;
    U32 powerThreshold = this->paramGet_POWER_THRESHOLD(valid);
    U32 criticalThreshold = this->paramGet_CRITICAL_THRESHOLD(valid);

    U32 vbatt = m_batteryState.get_vbatt_mV();

    if (vbatt < powerThreshold) {
        this->log_WARNING_HI_lowBattery(vbatt);
    }
    if (vbatt < criticalThreshold) {
        this->log_WARNING_HI_criticalBattery(vbatt);
    }

    m_powerState.set_vbatt_mV(vbatt);
    m_powerState.set_chargingState(m_batteryState.get_chargingState());
    m_powerState.set_faultStatus(m_batteryState.get_faultStatus());
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void EPSApplication ::SET_IC_REGISTER_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 FeatherCdh::BQ25756Reg regAddr,
                                                 U32 value) {
    this->setRegister_out(0, regAddr, value);
    this->log_ACTIVITY_HI_icRegisterForwarded(regAddr, value);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace FeatherCdh
