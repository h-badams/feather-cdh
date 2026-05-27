// ======================================================================
// \title  CommsApplication.hpp
// \author john_tensorflow
// \brief  hpp file for CommsApplication component implementation class
// ======================================================================

#ifndef FeatherCdh_CommsApplication_HPP
#define FeatherCdh_CommsApplication_HPP

#include "FeatherCdh/Components/CommsApplication/CommsApplicationComponentAc.hpp"

namespace FeatherCdh {

class CommsApplication final : public CommsApplicationComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct CommsApplication object
    CommsApplication(const char* const compName  //!< The component name
    );

    //! Destroy CommsApplication object
    ~CommsApplication();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for commsModeIn
    //!
    //! Mode command from SatStateMachine
    void commsModeIn_handler(FwIndexType portNum,  //!< The port number
                             const Comms::CommsMode& mode) override;

    //! Handler implementation for pingIn
    //!
    //! Health ping input from Svc.Health
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for schedIn
    //!
    //! 10 Hz rate group tick; drives state machine
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action emitSafeMode of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! Emit mode-transition event for Safe entry
    void FeatherCdh_CommsAppStateMachine_action_emitSafeMode(
        SmId smId,                                      //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action emitNormalMode of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! Emit mode-transition event for Normal entry
    void FeatherCdh_CommsAppStateMachine_action_emitNormalMode(
        SmId smId,                                      //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doSafeDrain of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! Fire comQueueRun every SAFE_DRAIN_DIVISOR ticks
    void FeatherCdh_CommsAppStateMachine_action_doSafeDrain(
        SmId smId,                                      //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doNormalDrain of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! Fire comQueueRun every tick
    void FeatherCdh_CommsAppStateMachine_action_doNormalDrain(
        SmId smId,                                      //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal  //!< The signal
        ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard isModeNormal of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! True if the switchMode signal carries Comms.CommsMode.Normal
    bool FeatherCdh_CommsAppStateMachine_guard_isModeNormal(
        SmId smId,                                       //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal,  //!< The signal
        const Comms::CommsMode& value                    //!< The value
    ) const override;

    //! Implementation for guard isModeSafe of state machine FeatherCdh_CommsAppStateMachine
    //!
    //! True if the switchMode signal carries Comms.CommsMode.Safe
    bool FeatherCdh_CommsAppStateMachine_guard_isModeSafe(
        SmId smId,                                       //!< The state machine id
        FeatherCdh_CommsAppStateMachine::Signal signal,  //!< The signal
        const Comms::CommsMode& value                    //!< The value
    ) const override;

  private:
    // ----------------------------------------------------------------------
    // Internal state
    // ----------------------------------------------------------------------

    Comms::CommsMode m_currentMode = Comms::CommsMode::Safe;
    U32 m_safeDrainCounter = 0;
};

}  // namespace FeatherCdh

#endif
