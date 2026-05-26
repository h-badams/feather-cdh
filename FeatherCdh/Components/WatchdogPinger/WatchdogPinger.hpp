// ======================================================================
// \title  WatchdogPinger.hpp
// \author john_tensorflow
// \brief  hpp file for WatchdogPinger component implementation class
// ======================================================================

#ifndef FeatherCdh_WatchdogPinger_HPP
#define FeatherCdh_WatchdogPinger_HPP

#include "FeatherCdh/Components/WatchdogPinger/WatchdogPingerComponentAc.hpp"

namespace FeatherCdh {

class WatchdogPinger final : public WatchdogPingerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct WatchdogPinger object
    WatchdogPinger(const char* const compName  //!< The component name
    );

    //! Destroy WatchdogPinger object
    ~WatchdogPinger();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for schedIn
    //!
    //! 10 Hz rate group tick from RateGroup10Hz
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;
};

}  // namespace FeatherCdh

#endif
