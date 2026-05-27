// ======================================================================
// \title  HC12Manager.cpp
// \author john_tensorflow
// \brief  cpp file for HC12Manager component implementation class
// ======================================================================

#include "FeatherCdh/Components/HC12Manager/HC12Manager.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

HC12Manager ::HC12Manager(const char* const compName) : HC12ManagerComponentBase(compName) {}

HC12Manager ::~HC12Manager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void HC12Manager ::dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    // Downlink: forward to UART driver, return buffer ownership to framer,
    // and signal tx status. On UART error, fire WARNING_HI and tear down to
    // RESET via the SM error signal (self-healing).
    const Drv::ByteStreamStatus sendStatus = this->drvSendOut_out(0, data);

    if (sendStatus == Drv::ByteStreamStatus::OP_OK) {
        this->m_bytesSent += static_cast<U32>(data.getSize());
        Fw::Success ok = Fw::Success::SUCCESS;
        this->comStatusOut_out(0, ok);
    } else {
        this->m_uartErrors++;
        this->log_WARNING_HI_uartSendError(sendStatus);
        Fw::Success fail = Fw::Success::FAILURE;
        this->comStatusOut_out(0, fail);
        this->sm_sendSignal_error();
    }

    // Always return downlink buffer ownership to the framer
    Fw::Buffer returned = data;
    ComCfg::FrameContext returnedContext = context;
    this->dataReturnOut_out(0, returned, returnedContext);
}

void HC12Manager ::dataReturnIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    // Uplink buffer return from frameAccumulator: hand ownership back to
    // the UART driver so the receive pool can re-use it.
    this->drvReceiveReturnOut_out(0, data);
}

void HC12Manager ::drvConnected_handler(FwIndexType portNum) {
    // Informational. Ready is signaled to the framer via comStatusOut on
    // RUN entry, independent of when the UART driver reports connected.
}

void HC12Manager ::drvReceiveIn_handler(FwIndexType portNum,
                                        Fw::Buffer& buffer,
                                        const Drv::ByteStreamStatus& status) {
    // Drop receive errors silently per design.
    if (status != Drv::ByteStreamStatus::OP_OK) {
        return;
    }

    this->m_bytesRecv += static_cast<U32>(buffer.getSize());
    ComCfg::FrameContext context;
    this->dataOut_out(0, buffer, context);
}

void HC12Manager ::schedIn_handler(FwIndexType portNum, U32 context) {
    this->sm_sendSignal_tick();
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void HC12Manager ::FeatherCdh_HC12StateMachine_action_doReset(SmId smId, FeatherCdh_HC12StateMachine::Signal signal) {
    // Pure delay tick; AT mode entry is unused in current design.
    this->sm_sendSignal_success();
}

void HC12Manager ::FeatherCdh_HC12StateMachine_action_doWaitReset(SmId smId,
                                                                  FeatherCdh_HC12StateMachine::Signal signal) {
    // One-tick settling delay.
    this->sm_sendSignal_success();
}

void HC12Manager ::FeatherCdh_HC12StateMachine_action_doConfigure(SmId smId,
                                                                  FeatherCdh_HC12StateMachine::Signal signal) {
    // Signal framer the link is ready, emit RUN-entry event, advance to RUN.
    this->log_ACTIVITY_HI_runEntry();
    Fw::Success ready = Fw::Success::SUCCESS;
    this->comStatusOut_out(0, ready);
    this->sm_sendSignal_success();
}

void HC12Manager ::FeatherCdh_HC12StateMachine_action_doRun(SmId smId, FeatherCdh_HC12StateMachine::Signal signal) {
    this->tlmWrite_BYTES_SENT(this->m_bytesSent);
    this->tlmWrite_BYTES_RECV(this->m_bytesRecv);
    this->tlmWrite_UART_ERRORS(this->m_uartErrors);
}

}  // namespace FeatherCdh
