// ======================================================================
// \title  HC12Manager.hpp
// \author john_tensorflow
// \brief  hpp file for HC12Manager component implementation class
// ======================================================================

#ifndef FeatherCdh_HC12Manager_HPP
#define FeatherCdh_HC12Manager_HPP

#include "FeatherCdh/Components/HC12Manager/HC12ManagerComponentAc.hpp"

namespace FeatherCdh {

class HC12Manager final : public HC12ManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct HC12Manager object
    HC12Manager(const char* const compName  //!< The component name
    );

    //! Destroy HC12Manager object
    ~HC12Manager();

#ifdef BUILD_UT
  public:
    using HC12ManagerComponentBase::dispatchCurrentMessages;
#endif

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for dataIn
    //!
    //! Data to be sent on the wire (coming in to the component)
    void dataIn_handler(FwIndexType portNum,  //!< The port number
                        Fw::Buffer& data,
                        const ComCfg::FrameContext& context) override;

    //! Handler implementation for dataReturnIn
    //!
    //! Port receiving back ownership of buffer sent out on dataOut
    void dataReturnIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& data,
                              const ComCfg::FrameContext& context) override;

    //! Handler implementation for drvConnected
    //!
    //! Ready signal when driver is connected
    void drvConnected_handler(FwIndexType portNum  //!< The port number
                              ) override;

    //! Handler implementation for drvReceiveIn
    //!
    //! Receive (read) data from driver.
    void drvReceiveIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& buffer,
                              const Drv::ByteStreamStatus& status) override;

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

    //! Implementation for action doReset of state machine FeatherCdh_HC12StateMachine
    //!
    //! Assert SET pin LOW and delay 40 ms for AT command mode entry
    void FeatherCdh_HC12StateMachine_action_doReset(SmId smId,  //!< The state machine id
                                                    FeatherCdh_HC12StateMachine::Signal signal  //!< The signal
                                                    ) override;

    //! Implementation for action doWaitReset of state machine FeatherCdh_HC12StateMachine
    //!
    //! One-tick settling delay after SET LOW before sending AT commands
    void FeatherCdh_HC12StateMachine_action_doWaitReset(SmId smId,  //!< The state machine id
                                                        FeatherCdh_HC12StateMachine::Signal signal  //!< The signal
                                                        ) override;

    //! Implementation for action doConfigure of state machine FeatherCdh_HC12StateMachine
    //!
    //! Send AT config commands; assert SET HIGH; delay 80 ms; emit READY on comStatusOut
    void FeatherCdh_HC12StateMachine_action_doConfigure(SmId smId,  //!< The state machine id
                                                        FeatherCdh_HC12StateMachine::Signal signal  //!< The signal
                                                        ) override;

    //! Implementation for action doRun of state machine FeatherCdh_HC12StateMachine
    //!
    //! Emit telemetry; byte passthrough handled directly by port handlers
    void FeatherCdh_HC12StateMachine_action_doRun(SmId smId,                                  //!< The state machine id
                                                  FeatherCdh_HC12StateMachine::Signal signal  //!< The signal
                                                  ) override;

  private:
    // Cumulative counters; preserved across SM resets per design.
    U32 m_bytesSent = 0;
    U32 m_bytesRecv = 0;
    U32 m_uartErrors = 0;
};

}  // namespace FeatherCdh

#endif
