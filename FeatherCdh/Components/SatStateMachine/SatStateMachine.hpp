// ======================================================================
// \title  SatStateMachine.hpp
// \author john_tensorflow
// \brief  hpp file for SatStateMachine component implementation class
// ======================================================================

#ifndef FeatherCdh_SatStateMachine_HPP
#define FeatherCdh_SatStateMachine_HPP

#include "FeatherCdh/Components/SatStateMachine/SatStateMachineComponentAc.hpp"

namespace FeatherCdh {

class SatStateMachine final : public SatStateMachineComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct SatStateMachine object
    SatStateMachine(const char* const compName  //!< The component name
    );

    //! Destroy SatStateMachine object
    ~SatStateMachine();

    //! Test-only public accessor for the SM state.
    //! The autocoded sm_getState() is protected; this wrapper exposes it so
    //! unit tests can assert on transitions.
    FeatherCdh::SatModeStateMachine_State getSmState() const {
        return this->sm_getState();
    }

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for pingIn
    //!
    //! Health ping input from Svc.Health
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for schedIn
    //!
    //! 1 Hz rate group tick; evaluates power state and emits mode telemetry
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SAFE_MODE
    //!
    //! Force transition to Safe mode from any state.
    void SAFE_MODE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                              U32 cmdSeq            //!< The command sequence number
                              ) override;

    //! Handler implementation for command SAFE_EXIT
    //!
    //! Transition from Safe to Standby. Rejected with EXECUTION_ERROR if not in Safe.
    void SAFE_EXIT_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                              U32 cmdSeq            //!< The command sequence number
                              ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action sendSafeMode of state machine FeatherCdh_SatModeStateMachine
    //!
    //! Send commsModeOut(Comms.CommsMode.Safe) and emit mode transition event
    void FeatherCdh_SatModeStateMachine_action_sendSafeMode(
        SmId smId,                                     //!< The state machine id
        FeatherCdh_SatModeStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action sendStandbyMode of state machine FeatherCdh_SatModeStateMachine
    //!
    //! Send commsModeOut(Comms.CommsMode.Normal) and emit mode transition event
    void FeatherCdh_SatModeStateMachine_action_sendStandbyMode(
        SmId smId,                                     //!< The state machine id
        FeatherCdh_SatModeStateMachine::Signal signal  //!< The signal
        ) override;
};

}  // namespace FeatherCdh

#endif
