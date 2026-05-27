#pragma once
#include "HC12ManagerTester.hpp"

namespace FeatherCdh {

class HC12ManagerTestState : public HC12ManagerTester {
  public:
    // State machine progression
    bool precondition__Sm__ResetTick() const;
    void action__Sm__ResetTick();

    bool precondition__Sm__WaitResetTick() const;
    void action__Sm__WaitResetTick();

    bool precondition__Sm__ConfigureNominal() const;
    void action__Sm__ConfigureNominal();

    bool precondition__Sm__RunTickTelemetry() const;
    void action__Sm__RunTickTelemetry();

    // Downlink (dataIn from framer)
    bool precondition__Downlink__Forward() const;
    void action__Downlink__Forward();

    bool precondition__Downlink__UartError() const;
    void action__Downlink__UartError();

    // Uplink (drvReceiveIn from UART driver)
    bool precondition__Uplink__Forward() const;
    void action__Uplink__Forward();

    bool precondition__Uplink__ReceiveError() const;
    void action__Uplink__ReceiveError();

    bool precondition__Uplink__ReturnBuffer() const;
    void action__Uplink__ReturnBuffer();

    // Informational
    bool precondition__DrvConnected__NoOp() const;
    void action__DrvConnected__NoOp();
};

}  // namespace FeatherCdh
