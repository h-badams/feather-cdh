#include "TestState.hpp"

namespace FeatherCdh {

// Note on assertions:
//   The autocoded ASSERT_* macros expand to `this->...` and cannot be called
//   as `fresh.ASSERT_*(...)`. We construct a `fresh` HC12ManagerTestState
//   inside each rule for isolation, then call the underlying protected
//   assert_*() methods on `fresh` directly. These are accessible from any
//   member function of HC12ManagerTestState because protected access is
//   permitted between instances of the same class.

// ---------------------------------------------------------------------------
// Rule: Sm::ResetTick
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Sm__ResetTick() const {
    return true;
}

void HC12ManagerTestState::action__Sm__ResetTick() {
    HC12ManagerTestState fresh;
    fresh.clearHistory();

    fresh.tickOnce();

    fresh.assert_from_setPin_size(__FILE__, __LINE__, 0);
    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_dataOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_dataReturnOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_drvReceiveReturnOut_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assertTlm_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Sm::WaitResetTick
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Sm__WaitResetTick() const {
    return true;
}

void HC12ManagerTestState::action__Sm__WaitResetTick() {
    HC12ManagerTestState fresh;
    fresh.driveToWaitReset();

    fresh.tickOnce();

    fresh.assert_from_setPin_size(__FILE__, __LINE__, 0);
    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assertTlm_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Sm::ConfigureNominal
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Sm__ConfigureNominal() const {
    return true;
}

void HC12ManagerTestState::action__Sm__ConfigureNominal() {
    HC12ManagerTestState fresh;
    fresh.driveToConfigure();

    fresh.tickOnce();

    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(Fw::Success::SUCCESS,
              fresh.fromPortHistory_comStatusOut->at(0).condition);
    fresh.assertEvents_runEntry_size(__FILE__, __LINE__, 1);
}

// ---------------------------------------------------------------------------
// Rule: Sm::RunTickTelemetry
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Sm__RunTickTelemetry() const {
    return true;
}

void HC12ManagerTestState::action__Sm__RunTickTelemetry() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();

    fresh.tickOnce();

    fresh.assert_from_setPin_size(__FILE__, __LINE__, 0);
    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);

    fresh.assertTlm_BYTES_SENT_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_BYTES_SENT(__FILE__, __LINE__, 0, 0u);
    fresh.assertTlm_BYTES_RECV_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_BYTES_RECV(__FILE__, __LINE__, 0, 0u);
    fresh.assertTlm_UART_ERRORS_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_UART_ERRORS(__FILE__, __LINE__, 0, 0u);
}

// ---------------------------------------------------------------------------
// Rule: Downlink::Forward
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Downlink__Forward() const {
    return true;
}

void HC12ManagerTestState::action__Downlink__Forward() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();
    fresh.m_sendStatus = Drv::ByteStreamStatus::OP_OK;

    U8 storage[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
    Fw::Buffer buffer(storage, sizeof(storage));
    ComCfg::FrameContext context;

    fresh.invoke_to_dataIn(0, buffer, context);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(buffer, fresh.fromPortHistory_drvSendOut->at(0).sendBuffer);

    fresh.assert_from_dataReturnOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(buffer, fresh.fromPortHistory_dataReturnOut->at(0).data);

    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(Fw::Success::SUCCESS,
              fresh.fromPortHistory_comStatusOut->at(0).condition);

    fresh.assertEvents_size(__FILE__, __LINE__, 0);

    fresh.clearHistory();
    fresh.tickOnce();
    fresh.assertTlm_BYTES_SENT_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_BYTES_SENT(__FILE__, __LINE__, 0,
                               static_cast<U32>(sizeof(storage)));
}

// ---------------------------------------------------------------------------
// Rule: Downlink::UartError
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Downlink__UartError() const {
    return true;
}

void HC12ManagerTestState::action__Downlink__UartError() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();
    fresh.m_sendStatus = Drv::ByteStreamStatus::OTHER_ERROR;

    U8 storage[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    Fw::Buffer buffer(storage, sizeof(storage));
    ComCfg::FrameContext context;

    fresh.invoke_to_dataIn(0, buffer, context);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 1);
    fresh.assert_from_dataReturnOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(buffer, fresh.fromPortHistory_dataReturnOut->at(0).data);

    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(Fw::Success::FAILURE,
              fresh.fromPortHistory_comStatusOut->at(0).condition);

    fresh.assertEvents_uartSendError_size(__FILE__, __LINE__, 1);
    fresh.assertEvents_uartSendError(__FILE__, __LINE__, 0,
                                     Drv::ByteStreamStatus::OTHER_ERROR);

    // SM transitioned to RESET via error signal. RESET doesn't emit telemetry.
    fresh.clearHistory();
    fresh.tickOnce();
    fresh.assertTlm_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: Uplink::Forward
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Uplink__Forward() const {
    return true;
}

void HC12ManagerTestState::action__Uplink__Forward() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();

    U8 storage[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    Fw::Buffer buffer(storage, sizeof(storage));
    ComCfg::FrameContext expectedContext;

    fresh.invoke_to_drvReceiveIn(0, buffer, Drv::ByteStreamStatus::OP_OK);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_dataOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(buffer, fresh.fromPortHistory_dataOut->at(0).data);

    fresh.assertEvents_size(__FILE__, __LINE__, 0);

    fresh.clearHistory();
    fresh.tickOnce();
    fresh.assertTlm_BYTES_RECV_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_BYTES_RECV(__FILE__, __LINE__, 0,
                               static_cast<U32>(sizeof(storage)));
}

// ---------------------------------------------------------------------------
// Rule: Uplink::ReceiveError
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Uplink__ReceiveError() const {
    return true;
}

void HC12ManagerTestState::action__Uplink__ReceiveError() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();

    U8 storage[4] = {0};
    Fw::Buffer buffer(storage, sizeof(storage));

    fresh.invoke_to_drvReceiveIn(0, buffer, Drv::ByteStreamStatus::RECV_NO_DATA);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_dataOut_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);

    fresh.clearHistory();
    fresh.tickOnce();
    fresh.assertTlm_BYTES_RECV_size(__FILE__, __LINE__, 1);
    fresh.assertTlm_BYTES_RECV(__FILE__, __LINE__, 0, 0u);
}

// ---------------------------------------------------------------------------
// Rule: Uplink::ReturnBuffer
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__Uplink__ReturnBuffer() const {
    return true;
}

void HC12ManagerTestState::action__Uplink__ReturnBuffer() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();

    U8 storage[4] = {0x77, 0x88, 0x99, 0xAA};
    Fw::Buffer buffer(storage, sizeof(storage));
    ComCfg::FrameContext context;

    fresh.invoke_to_dataReturnIn(0, buffer, context);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_drvReceiveReturnOut_size(__FILE__, __LINE__, 1);
    ASSERT_EQ(buffer, fresh.fromPortHistory_drvReceiveReturnOut->at(0).fwBuffer);

    fresh.assertEvents_size(__FILE__, __LINE__, 0);
}

// ---------------------------------------------------------------------------
// Rule: DrvConnected::NoOp
// ---------------------------------------------------------------------------

bool HC12ManagerTestState::precondition__DrvConnected__NoOp() const {
    return true;
}

void HC12ManagerTestState::action__DrvConnected__NoOp() {
    HC12ManagerTestState fresh;
    fresh.driveToRun();

    fresh.invoke_to_drvConnected(0);
    fresh.component.dispatchCurrentMessages();

    fresh.assert_from_comStatusOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_drvSendOut_size(__FILE__, __LINE__, 0);
    fresh.assert_from_dataOut_size(__FILE__, __LINE__, 0);
    fresh.assertEvents_size(__FILE__, __LINE__, 0);
    fresh.assertTlm_size(__FILE__, __LINE__, 0);
}

}  // namespace FeatherCdh
