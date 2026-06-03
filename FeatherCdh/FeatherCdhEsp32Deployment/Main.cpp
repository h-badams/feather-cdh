// ======================================================================
// \title  Main.cpp
// \brief  F' entry point for ESP32-S3 Arduino target
// ======================================================================
#include <Arduino.h>
#include <FeatherCdh/FeatherCdhEsp32Deployment/Top/FeatherCdhEsp32DeploymentTopology.hpp>

void setup() {
    FeatherCdh::TopologyState inputs;
    FeatherCdh::setupTopology(inputs);
    // HardwareRateDriver timer ISR starts firing once setupTopology() completes.
    // Active components run as FreeRTOS tasks started inside startTasks().
}

void loop() {
    // All work is driven by FreeRTOS tasks (active components) and the
    // HardwareRateDriver hardware timer ISR. The Arduino loop() stays idle.
    delay(1000);
}
