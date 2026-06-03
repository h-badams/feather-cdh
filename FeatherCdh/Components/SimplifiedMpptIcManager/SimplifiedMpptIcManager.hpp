// ======================================================================
// \title  SimplifiedMpptIcManager.hpp
// \author john_tensorflow
// \brief  hpp file for SimplifiedMpptIcManager component implementation class
// ======================================================================

#ifndef FeatherCdh_SimplifiedMpptIcManager_HPP
#define FeatherCdh_SimplifiedMpptIcManager_HPP

#include "FeatherCdh/Components/SimplifiedMpptIcManager/SimplifiedMpptIcManagerComponentAc.hpp"

namespace FeatherCdh {

class SimplifiedMpptIcManager final : public SimplifiedMpptIcManagerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct SimplifiedMpptIcManager object
    SimplifiedMpptIcManager(const char* const compName  //!< The component name
    );

    //! Destroy SimplifiedMpptIcManager object
    ~SimplifiedMpptIcManager();

  private:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    //! BQ25756E I2C device address
    static constexpr U8 BQ25756E_ADDR = 0x6B;

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for intPin
    //!
    //! BQ25756E INT pin interrupt — driven by LinuxGpioDriver.gpioInterrupt.
    //! INT is a 256 µs active-low pulse; it self-deasserts and requires no register read to release.
    //! Handler reads FAULT_STATUS and FAULT_FLAG to capture the source, then emits faultInterrupt.
    void intPin_handler(FwIndexType portNum,     //!< The port number
                        Os::RawTime& cycleStart  //!< Cycle start timestamp
                        ) override;

    //! Handler implementation for schedIn
    //!
    //! 1 Hz rate group tick; reads all measurement registers and emits BatteryState
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

    //! Handler implementation for setRegister
    //!
    //! Register write forwarded from EPSApplication; always honored
    void setRegister_handler(FwIndexType portNum,  //!< The port number
                             const FeatherCdh::BQ25756Reg& regAddr,
                             U32 value) override;

    // ----------------------------------------------------------------------
    // Private helpers
    // ----------------------------------------------------------------------

    //! Returns true if reg is a 16-bit (2-byte) register, false if 8-bit
    static bool isReg16Bit(BQ25756Reg reg);

    //! Write an 8-bit register. Emits i2cWriteError on failure.
    Drv::I2cStatus writeReg8(BQ25756Reg reg, U8 value);

    //! Write a 16-bit register (big-endian, MSB first). Emits i2cWriteError on failure.
    Drv::I2cStatus writeReg16(BQ25756Reg reg, U16 value);

    //! Read an 8-bit register into value. Emits i2cReadError on failure.
    Drv::I2cStatus readReg8(BQ25756Reg reg, U8& value);

    //! Read a 16-bit register (big-endian) into value. Emits i2cReadError on failure.
    Drv::I2cStatus readReg16(BQ25756Reg reg, U16& value);
};

}  // namespace FeatherCdh

#endif
