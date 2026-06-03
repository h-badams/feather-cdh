// ======================================================================
// \title  MpptIcManager.hpp
// \author john_tensorflow
// \brief  hpp file for MpptIcManager component implementation class
// ======================================================================

#ifndef FeatherCdh_MpptIcManager_HPP
#define FeatherCdh_MpptIcManager_HPP

#include "FeatherCdh/Components/MpptIcManager/MpptIcManagerComponentAc.hpp"

namespace FeatherCdh {

class MpptIcManager final : public MpptIcManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct MpptIcManager object
    MpptIcManager(const char* const compName  //!< The component name
    );

    //! Destroy MpptIcManager object
    ~MpptIcManager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for intPin
    //!
    //! BQ25756E INT pin interrupt — asserted on fault condition; driven by LinuxGpioDriver.gpioInterrupt
    void intPin_handler(FwIndexType portNum,     //!< The port number
                        Os::RawTime& cycleStart  //!< Cycle start timestamp
                        ) override;

    //! Handler implementation for schedIn
    //!
    //! 1 Hz rate group tick; drives state machine
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

    //! Handler implementation for setRegister
    //!
    //! Register write forwarded from EPSApplication; dropped with WARNING_LO during non-RUN states
    void setRegister_handler(FwIndexType portNum,  //!< The port number
                             const FeatherCdh::BQ25756Reg& regAddr,
                             U32 value) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doReset of state machine FeatherCdh_MpptIcStateMachine
    //!
    //! Write REG_RST=1 to software-reset all BQ25756E registers
    void FeatherCdh_MpptIcStateMachine_action_doReset(SmId smId,  //!< The state machine id
                                                      FeatherCdh_MpptIcStateMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action checkReset of state machine FeatherCdh_MpptIcStateMachine
    //!
    //! Wait one tick for registers to settle after software reset
    void FeatherCdh_MpptIcStateMachine_action_checkReset(SmId smId,  //!< The state machine id
                                                         FeatherCdh_MpptIcStateMachine::Signal signal  //!< The signal
                                                         ) override;

    //! Implementation for action doEnable of state machine FeatherCdh_MpptIcStateMachine
    //!
    //! Read PART_INFO (0x3D) to verify BQ25756E device ID
    void FeatherCdh_MpptIcStateMachine_action_doEnable(SmId smId,  //!< The state machine id
                                                       FeatherCdh_MpptIcStateMachine::Signal signal  //!< The signal
                                                       ) override;

    //! Implementation for action doConfigure of state machine FeatherCdh_MpptIcStateMachine
    //!
    //! Write watchdog disable, ADC enable, and charging config registers; read back to verify
    void FeatherCdh_MpptIcStateMachine_action_doConfigure(SmId smId,  //!< The state machine id
                                                          FeatherCdh_MpptIcStateMachine::Signal signal  //!< The signal
                                                          ) override;

    //! Implementation for action doRun of state machine FeatherCdh_MpptIcStateMachine
    //!
    //! Read all measurement and status registers; assemble and push BatteryState
    void FeatherCdh_MpptIcStateMachine_action_doRun(SmId smId,  //!< The state machine id
                                                    FeatherCdh_MpptIcStateMachine::Signal signal  //!< The signal
                                                    ) override;
};

}  // namespace FeatherCdh

#endif
