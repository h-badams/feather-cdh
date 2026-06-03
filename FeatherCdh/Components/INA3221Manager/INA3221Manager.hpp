// ======================================================================
// \title  INA3221Manager.hpp
// \author john_tensorflow
// \brief  hpp file for INA3221Manager component implementation class
// ======================================================================

#ifndef FeatherCdh_INA3221Manager_HPP
#define FeatherCdh_INA3221Manager_HPP

#include "FeatherCdh/Components/INA3221Manager/INA3221ManagerComponentAc.hpp"

namespace FeatherCdh {

class INA3221Manager final : public INA3221ManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct INA3221Manager object
    INA3221Manager(const char* const compName  //!< The component name
    );

    //! Destroy INA3221Manager object
    ~INA3221Manager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for schedIn
    //!
    //! 1 Hz rate group tick; drives state machine
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doReset of state machine FeatherCdh_INA3221StateMachine
    //!
    //! Assert SET — no-op for INA3221 (no hardware reset GPIO)
    void FeatherCdh_INA3221StateMachine_action_doReset(SmId smId,  //!< The state machine id
                                                       FeatherCdh_INA3221StateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action checkReset of state machine FeatherCdh_INA3221StateMachine
    //!
    //! Wait one tick for IC settling
    void FeatherCdh_INA3221StateMachine_action_checkReset(SmId smId,  //!< The state machine id
                                                          FeatherCdh_INA3221StateMachine::Signal signal  //!< The signal
                                                          ) override;

    //! Implementation for action doEnable of state machine FeatherCdh_INA3221StateMachine
    //!
    //! Read Manufacturer ID register to verify I2C connectivity
    void FeatherCdh_INA3221StateMachine_action_doEnable(SmId smId,  //!< The state machine id
                                                        FeatherCdh_INA3221StateMachine::Signal signal  //!< The signal
                                                        ) override;

    //! Implementation for action doConfigure of state machine FeatherCdh_INA3221StateMachine
    //!
    //! Write configuration register (averaging, conversion time, channel enable)
    void FeatherCdh_INA3221StateMachine_action_doConfigure(
        SmId smId,                                     //!< The state machine id
        FeatherCdh_INA3221StateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doRun of state machine FeatherCdh_INA3221StateMachine
    //!
    //! Read all six channel registers and emit telemetry
    void FeatherCdh_INA3221StateMachine_action_doRun(SmId smId,  //!< The state machine id
                                                     FeatherCdh_INA3221StateMachine::Signal signal  //!< The signal
                                                     ) override;
};

}  // namespace FeatherCdh

#endif
