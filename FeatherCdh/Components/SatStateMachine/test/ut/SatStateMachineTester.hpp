#pragma once
#include "SatStateMachineGTestBase.hpp"
#include "FeatherCdh/Components/SatStateMachine/SatStateMachine.hpp"

namespace FeatherCdh {

class SatStateMachineTester : public SatStateMachineGTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static constexpr FwSizeType MAX_HISTORY_SIZE = 64;

    // Opcodes assigned by FPP in declaration order (SAFE_MODE first, SAFE_EXIT second)
    static constexpr FwOpcodeType OPCODE_SAFE_MODE = 0;
    static constexpr FwOpcodeType OPCODE_SAFE_EXIT = 1;

    SatStateMachine component;

    // PowerState returned by from_powerStateGet_handler; set before each tick
    PowerState m_mockPowerState;

    SatStateMachineTester()
        : SatStateMachineGTestBase("SatStateMachineTester", MAX_HISTORY_SIZE),
          component("SatStateMachine") {
        // Custom init order: the SM's initial-state entry action (sendSafeMode)
        // calls commsModeOut_out during component.init(), which asserts if the
        // output port isn't yet connected. So we wire ports first, then init
        // the component. This means we cannot use the auto-generated
        // initComponents() (which does this->init() + component.init() back-to-back).
        this->init();
        this->connectPorts();
        this->component.init(TEST_INSTANCE_QUEUE_DEPTH, TEST_INSTANCE_ID);
        this->clearHistory();
    }

    // Body provided by UT_AUTO_HELPERS (SatStateMachineTesterHelpers.cpp)
    void connectPorts();
    // initComponents() is also auto-generated but intentionally unused here —
    // see constructor note above.
    void initComponents();

    // -----------------------------------------------------------------------
    // Helpers — public so TestMain can call them on a TestState object
    // -----------------------------------------------------------------------

    // Invoke one 1 Hz schedIn tick and drain all resulting queue messages
    // (schedIn message + any SM signal message it enqueues).
    void sendTick() {
        this->invoke_to_schedIn(0, 0);
        SatStateMachineTesterBase::dispatchCurrentMessages(this->component);
    }

    void setThreshold(U32 threshold) {
        this->paramSet_CRITICAL_THRESHOLD(threshold, Fw::ParamValid::VALID);
        this->component.loadParameters();
    }

    // Transition from SAFE to STANDBY via SAFE_EXIT command; clears history.
    // Caller must guarantee the SM is currently in SAFE.
    void transitionToStandby() {
        this->sendCmd_SAFE_EXIT(TEST_INSTANCE_ID, 1);
        SatStateMachineTesterBase::dispatchCurrentMessages(this->component);
        this->clearHistory();
    }

  protected:
    // -----------------------------------------------------------------------
    // Output port handlers (component → tester)
    // -----------------------------------------------------------------------

    PowerState from_powerStateGet_handler(FwIndexType portNum) override {
        this->pushFromPortEntry_powerStateGet();
        return m_mockPowerState;
    }

    void from_commsModeOut_handler(FwIndexType portNum,
                                   const Comms::CommsMode& mode) override {
        this->pushFromPortEntry_commsModeOut(mode);
    }

    void from_pingOut_handler(FwIndexType portNum, U32 key) override {
        this->pushFromPortEntry_pingOut(key);
    }
};

}  // namespace FeatherCdh
