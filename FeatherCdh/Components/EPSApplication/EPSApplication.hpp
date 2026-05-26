// ======================================================================
// \title  EPSApplication.hpp
// \author john_tensorflow
// \brief  hpp file for EPSApplication component implementation class
// ======================================================================

#ifndef FeatherCdh_EPSApplication_HPP
#define FeatherCdh_EPSApplication_HPP

#include "FeatherCdh/Components/EPSApplication/EPSApplicationComponentAc.hpp"

namespace FeatherCdh {

class EPSApplication final : public EPSApplicationComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct EPSApplication object
    EPSApplication(const char* const compName  //!< The component name
    );

    //! Destroy EPSApplication object
    ~EPSApplication();

  private:
    BatteryState m_batteryState;
    PowerState m_powerState;

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for batteryStateIn
    //!
    //! Battery state pushed by MpptIcManager each tick; cached for next schedIn processing
    void batteryStateIn_handler(FwIndexType portNum,  //!< The port number
                                FeatherCdh::BatteryState& batteryState) override;

    //! Handler implementation for pingIn
    //!
    //! Health ping input from Svc.Health
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for powerStateGet
    //!
    //! Sync get called by SatStateMachine on its own thread; returns cached PowerState
    FeatherCdh::PowerState powerStateGet_handler(FwIndexType portNum  //!< The port number
                                                 ) override;

    //! Handler implementation for schedIn
    //!
    //! 1 Hz rate group tick; assembles and caches PowerState
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SET_IC_REGISTER
    //!
    //! Write a BQ25756E register via MpptIcManager. Forwarded unconditionally.
    void SET_IC_REGISTER_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                    U32 cmdSeq,           //!< The command sequence number
                                    FeatherCdh::BQ25756Reg regAddr,
                                    U32 value) override;
};

}  // namespace FeatherCdh

#endif
