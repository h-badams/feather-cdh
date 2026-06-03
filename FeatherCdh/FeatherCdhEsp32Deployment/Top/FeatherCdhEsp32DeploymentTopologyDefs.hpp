// ======================================================================
// \title  FeatherCdhEsp32DeploymentTopologyDefs.hpp
// \brief  required header file containing the required definitions for the topology autocoder
// ======================================================================
#ifndef FEATHERCDHESP32DEPLOYMENT_TOPOLOGYDEFS_HPP
#define FEATHERCDHESP32DEPLOYMENT_TOPOLOGYDEFS_HPP

// Subtopology PingEntries includes
#include "Svc/Subtopologies/CdhCore/PingEntries.hpp"
#include "Svc/Subtopologies/ComCcsds/PingEntries.hpp"
#include "Svc/Subtopologies/FileHandling/PingEntries.hpp"

// SubtopologyTopologyDefs includes
#include "Svc/Subtopologies/CdhCore/SubtopologyTopologyDefs.hpp"
#include "Svc/Subtopologies/ComCcsds/SubtopologyTopologyDefs.hpp"
#include "Svc/Subtopologies/FileHandling/SubtopologyTopologyDefs.hpp"

// ComCcsds Enum Includes
#include "Svc/Subtopologies/ComCcsds/Ports_ComPacketQueueEnumAc.hpp"
#include "Svc/Subtopologies/ComCcsds/Ports_ComBufferQueueEnumAc.hpp"

// Include autocoded FPP constants
#include "FeatherCdh/FeatherCdhEsp32Deployment/Top/FppConstantsAc.hpp"

namespace PingEntries {
    // Active rate groups
    namespace FeatherCdh_rateGroup10Hz    { enum { WARN = 3, FATAL = 5 }; }
    namespace FeatherCdh_rateGroup1Hz     { enum { WARN = 3, FATAL = 5 }; }
    // Active application components (health-monitored)
    namespace FeatherCdh_satStateMachine  { enum { WARN = 3, FATAL = 5 }; }
    namespace FeatherCdh_epsApplication   { enum { WARN = 3, FATAL = 5 }; }
    namespace FeatherCdh_commsApplication { enum { WARN = 3, FATAL = 5 }; }
    // mpptIcManager and hc12Manager are NOT health-monitored (hardware managers)
}  // namespace PingEntries

namespace FeatherCdh {

// TopologyState carries any runtime configuration needed during setupTopology().
// No UART device path or baud rate — Arduino drivers are configured via hardcoded
// peripheral references (Wire, Serial1) in configureTopology().
struct TopologyState {
    CdhCore::SubtopologyState cdhCore;
    ComCcsds::SubtopologyState comCcsds;
    FileHandling::SubtopologyState fileHandling;
};

namespace PingEntries = ::PingEntries;
}  // namespace FeatherCdh

#endif
