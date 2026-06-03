// ======================================================================
// \title  FeatherCdhEsp32DeploymentTopology.cpp
// \brief  Topology instantiation code for the ESP32-S3 Arduino deployment
// ======================================================================
#include <FeatherCdh/FeatherCdhEsp32Deployment/Top/FeatherCdhEsp32DeploymentTopologyAc.hpp>
// Uncomment if the TlmPacketizer packet list header is needed directly in this file:
// #include <FeatherCdh/FeatherCdhEsp32Deployment/Top/FeatherCdhEsp32DeploymentPacketsAc.hpp>

// Arduino peripheral headers
#include <Wire.h>
#include <Arduino.h>

namespace FeatherCdh {

// 50 Hz base clock divided down to 10 Hz (÷5) and 1 Hz (÷50)
Svc::RateGroupDriver::DividerSet rateGroupDivisors{{{5, 0}, {50, 0}}};

U32 rateGroup10HzContext[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup1HzContext[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX]  = {};

// -----------------------------------------------------------------------
// Pin assignments — adjust to match actual board wiring
// -----------------------------------------------------------------------

// HC-12 SET pin: output, HIGH = transparent mode, LOW = AT command mode
static constexpr U32 HC12_SET_PIN     = 4;

// HC-12 UART on Serial1 — ESP32-S3 pins are remappable; choose free pins
static constexpr int HC12_UART_RX_PIN = 16;
static constexpr int HC12_UART_TX_PIN = 17;

// BQ25756E uses Wire (I2C0).  Default ESP32-S3 Dev Module I2C pins:
//   SDA = GPIO8,  SCL = GPIO9  (check your board's pinout)

// -----------------------------------------------------------------------

void configureTopology() {
    // Rate group driver divisor table
    rateGroupDriver.configure(rateGroupDivisors);
    rateGroup10Hz.configure(rateGroup10HzContext, FW_NUM_ARRAY_ELEMENTS(rateGroup10HzContext));
    rateGroup1Hz.configure(rateGroup1HzContext,   FW_NUM_ARRAY_ELEMENTS(rateGroup1HzContext));

    // I2C for BQ25756E — Wire must be started before the I2cDriver uses it
    Wire.begin();
    i2cDriverBms.open(&Wire);

    // HC-12 UART at 9600 8N1 on Serial1
    Serial1.begin(9600, SERIAL_8N1, HC12_UART_RX_PIN, HC12_UART_TX_PIN);
    streamDriver.configure(&Serial1);

    // HC-12 SET pin — direction only; HC12Manager drives the logic level via setPin port
    gpioDriverSetPin.open(HC12_SET_PIN, Arduino::GpioDriver::OUT);
}

void setupTopology(const TopologyState& state) {
    initComponents(state);
    setBaseIds();
    connectComponents();
    regCommands();
    configComponents(state);
    configureTopology();
    loadParameters();
    startTasks(state);
    // HardwareRateDriver begins firing its timer ISR once the component is initialised
    // inside initComponents() — no explicit start call is required here.
}

// HardwareRateDriver is driven by a hardware timer ISR, not a blocking loop.
// startRateGroups() and stopRateGroups() are no-ops for this deployment.
void startRateGroups(const Fw::TimeInterval&) {}

void stopRateGroups() {}

void teardownTopology(const TopologyState& state) {
    // Stop and join all active component tasks
    stopTasks(state);
    freeThreads(state);
    tearDownComponents(state);
    deinitComponents(state);
}

}  // namespace FeatherCdh
