// ======================================================================
// \title  FeatherCdhPartialDeploymentTopology.hpp
// \brief header file containing the topology instantiation definitions
//
// ======================================================================
#ifndef FEATHERCDH_FEATHERCDHPARTIALDEPLOY_TOPOLOGY_HPP
#define FEATHERCDH_FEATHERCDHPARTIALDEPLOY_TOPOLOGY_HPP

#include <FeatherCdh/FeatherCdhPartialDeployment/Top/FeatherCdhPartialDeploymentTopologyDefs.hpp>

namespace FeatherCdh {

//! \brief initialize and run the F´ topology
void setupTopology(const TopologyState& state);

//! \brief teardown the F´ topology
void teardownTopology(const TopologyState& state);

//! \brief cycle the rate group driver via the system timer (blocking)
void startRateGroups(const Fw::TimeInterval& interval);

//! \brief stop the rate groups cycle started by startRateGroups
void stopRateGroups();

}  // namespace FeatherCdh
#endif
