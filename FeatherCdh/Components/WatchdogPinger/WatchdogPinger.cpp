// ======================================================================
// \title  WatchdogPinger.cpp
// \author john_tensorflow
// \brief  cpp file for WatchdogPinger component implementation class
// ======================================================================

#include "FeatherCdh/Components/WatchdogPinger/WatchdogPinger.hpp"

namespace FeatherCdh {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WatchdogPinger ::WatchdogPinger(const char* const compName) : WatchdogPingerComponentBase(compName) {}

WatchdogPinger ::~WatchdogPinger() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void WatchdogPinger ::schedIn_handler(FwIndexType portNum, U32 context) {
    watchdogPing_out(0, Fw::Logic::HIGH);
    watchdogPing_out(0, Fw::Logic::LOW);
}

}  // namespace FeatherCdh
