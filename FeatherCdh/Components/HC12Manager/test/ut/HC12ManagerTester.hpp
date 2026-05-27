#pragma once
#include "HC12ManagerGTestBase.hpp"
#include "FeatherCdh/Components/HC12Manager/HC12Manager.hpp"

namespace FeatherCdh {

class HC12ManagerTester : public HC12ManagerGTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static constexpr U32 MAX_HISTORY_SIZE = 64;

    HC12Manager component;

    // Injectable stub return values
    Drv::GpioStatus m_gpioStatus = Drv::GpioStatus::OP_OK;
    Drv::ByteStreamStatus m_sendStatus = Drv::ByteStreamStatus::OP_OK;

    HC12ManagerTester()
        : HC12ManagerGTestBase("HC12ManagerTester", MAX_HISTORY_SIZE),
          component("HC12Manager") {
        this->initComponents();
        this->connectPorts();
        this->clearHistory();
    }

    // Bodies provided by UT_AUTO_HELPERS
    void connectPorts();
    void initComponents();

    // ------------------------------------------------------------------
    // Helpers for rules
    // ------------------------------------------------------------------

    // Drive one schedIn tick and drain all queued state-machine signals.
    void tickOnce() {
        this->invoke_to_schedIn(0, 0);
        this->component.dispatchCurrentMessages();
    }

    // Walk the SM from initial RESET to WAIT_RESET, then clear history.
    void driveToWaitReset() {
        this->tickOnce();  // RESET -> WAIT_RESET
        this->clearHistory();
    }

    // Walk the SM from initial RESET to CONFIGURE, then clear history.
    void driveToConfigure() {
        this->tickOnce();  // RESET -> WAIT_RESET
        this->tickOnce();  // WAIT_RESET -> CONFIGURE
        this->clearHistory();
    }

    // Walk the SM from initial RESET to RUN, then clear history.
    void driveToRun() {
        this->tickOnce();  // RESET -> WAIT_RESET
        this->tickOnce();  // WAIT_RESET -> CONFIGURE
        this->tickOnce();  // CONFIGURE -> RUN
        this->clearHistory();
    }

  protected:
    // ------------------------------------------------------------------
    // Output port handler overrides
    // setPin: kept in FPP but doConfigure/doReset don't call it; we still
    // record any call so accidental future use surfaces in history.
    // ------------------------------------------------------------------

    Drv::GpioStatus from_setPin_handler(FwIndexType portNum,
                                        const Fw::Logic& state) override {
        this->pushFromPortEntry_setPin(state);
        return this->m_gpioStatus;
    }

    Drv::ByteStreamStatus from_drvSendOut_handler(FwIndexType portNum,
                                                  Fw::Buffer& sendBuffer) override {
        this->pushFromPortEntry_drvSendOut(sendBuffer);
        return this->m_sendStatus;
    }

    void from_drvReceiveReturnOut_handler(FwIndexType portNum,
                                          Fw::Buffer& fwBuffer) override {
        this->pushFromPortEntry_drvReceiveReturnOut(fwBuffer);
    }

    void from_dataOut_handler(FwIndexType portNum,
                              Fw::Buffer& data,
                              const ComCfg::FrameContext& context) override {
        this->pushFromPortEntry_dataOut(data, context);
    }

    void from_dataReturnOut_handler(FwIndexType portNum,
                                    Fw::Buffer& data,
                                    const ComCfg::FrameContext& context) override {
        this->pushFromPortEntry_dataReturnOut(data, context);
    }

    void from_comStatusOut_handler(FwIndexType portNum,
                                   Fw::Success& condition) override {
        this->pushFromPortEntry_comStatusOut(condition);
    }
};

}  // namespace FeatherCdh
