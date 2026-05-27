#pragma once
#include "CommsApplicationTester.hpp"

namespace FeatherCdh {

class CommsApplicationTestState : public CommsApplicationTester {
  public:
    // State machine
    bool precondition__Sm__SafeOnBoot() const;
    void action__Sm__SafeOnBoot();

    bool precondition__Sm__SafeToNormal() const;
    void action__Sm__SafeToNormal();

    bool precondition__Sm__NormalToSafe() const;
    void action__Sm__NormalToSafe();

    bool precondition__Sm__SafeIgnoresSafe() const;
    void action__Sm__SafeIgnoresSafe();

    bool precondition__Sm__NormalIgnoresNormal() const;
    void action__Sm__NormalIgnoresNormal();

    // ComQueue drain
    bool precondition__Drain__NormalEveryTick() const;
    void action__Drain__NormalEveryTick();

    bool precondition__Drain__SafeEveryDivisor() const;
    void action__Drain__SafeEveryDivisor();

    bool precondition__Drain__SafeSuppressesBetween() const;
    void action__Drain__SafeSuppressesBetween();

    bool precondition__Drain__ModeSwitchResetsCounter() const;
    void action__Drain__ModeSwitchResetsCounter();

    // Telemetry
    bool precondition__Tlm__CommsModeEachTick() const;
    void action__Tlm__CommsModeEachTick();

    // Health ping
    bool precondition__Ping__Echo() const;
    void action__Ping__Echo();
};

}  // namespace FeatherCdh
