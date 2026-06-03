// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application. Intended for CLI-based systems (Linux, macOS)
//
// ======================================================================
#include <FeatherCdh/FeatherCdhPartialDeployment/Top/FeatherCdhPartialDeploymentTopology.hpp>
#include <Os/Os.hpp>
#include <signal.h>

static void signalHandler(int signum) {
    FeatherCdh::stopRateGroups();
}

int main(int argc, char* argv[]) {
    Os::init();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    FeatherCdh::TopologyState inputs;
    FeatherCdh::setupTopology(inputs);
    FeatherCdh::startRateGroups(Fw::TimeInterval(0, 20000));  // 20 ms = 50 Hz base rate
    FeatherCdh::teardownTopology(inputs);
    return 0;
}
