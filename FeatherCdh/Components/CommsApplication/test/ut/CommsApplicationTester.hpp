#pragma once
#include "CommsApplicationGTestBase.hpp"
#include "FeatherCdh/Components/CommsApplication/CommsApplication.hpp"

namespace FeatherCdh {

class CommsApplicationTester : public CommsApplicationGTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static constexpr U32 MAX_HISTORY_SIZE = 64;

    static constexpr U32 SAFE_DRAIN_DIVISOR = 10;

    CommsApplication component;

    CommsApplicationTester()
        : CommsApplicationGTestBase("CommsApplicationTester", MAX_HISTORY_SIZE),
          component("CommsApplication") {
        this->initComponents();
        this->connectPorts();
        // Load default parameter so paramGet_SAFE_DRAIN_DIVISOR returns a deterministic value.
        // Local copy avoids odr-use of the static constexpr member (needed pre-C++17).
        const U32 divisor = SAFE_DRAIN_DIVISOR;
        this->paramSet_SAFE_DRAIN_DIVISOR(divisor, Fw::ParamValid::VALID);
        this->component.loadParameters();
        this->clearHistory();
    }

    // Bodies provided by UT_AUTO_HELPERS
    void connectPorts();
    void initComponents();

    // ------------------------------------------------------------------
    // Helpers for rules
    // ------------------------------------------------------------------

    // Drain the queue until truly empty. dispatchCurrentMessages snapshots the
    // queue size at entry, so signals enqueued by sm_sendSignal_* during the
    // commsModeIn / schedIn handler dispatch are left behind for the next call.
    // It returns MSG_DISPATCH_EMPTY only when the snapshot was 0, so looping
    // until EMPTY drains everything without ever calling doDispatch on an empty
    // queue (which would block).
    void drainAll() {
        while (CommsApplicationTesterBase::dispatchCurrentMessages(this->component) !=
               CommsApplicationComponentBase::MSG_DISPATCH_EMPTY) {
        }
    }

    // Drive one schedIn tick and drain queued SM signals.
    void tickOnce() {
        this->invoke_to_schedIn(0, 0);
        this->drainAll();
    }

    // Send a mode command and drain queued SM signals.
    void sendModeCommand(Comms::CommsMode mode) {
        this->invoke_to_commsModeIn(0, mode);
        this->drainAll();
    }

    // Drive SM to NORMAL, then clear history.
    void driveToNormal() {
        this->sendModeCommand(Comms::CommsMode::Normal);
        this->clearHistory();
    }

  protected:
    void from_comQueueRun_handler(FwIndexType portNum, U32 context) override {
        this->pushFromPortEntry_comQueueRun(context);
    }

    void from_pingOut_handler(FwIndexType portNum, U32 key) override {
        this->pushFromPortEntry_pingOut(key);
    }
};

}  // namespace FeatherCdh
