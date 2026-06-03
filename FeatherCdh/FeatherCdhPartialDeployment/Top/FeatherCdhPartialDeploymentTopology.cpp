// ======================================================================
// \title  FeatherCdhPartialDeploymentTopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
#include <FeatherCdh/FeatherCdhPartialDeployment/Top/FeatherCdhPartialDeploymentTopologyAc.hpp>

// Necessary project-specified types
#include <Fw/Types/MallocAllocator.hpp>

// Public functions for use in main program are namespaced with deployment module FeatherCdh
// This is also the namespace where the topology components are instantiated by FPP.
namespace FeatherCdh {

// Instantiate a malloc allocator for cmdSeq buffer allocation
Fw::MallocAllocator mallocator;

// 50 Hz base clock divided down to 10 Hz (÷5) and 1 Hz (÷50)
Svc::RateGroupDriver::DividerSet rateGroupDivisors{{{5, 0}, {50, 0}}};

// Rate groups may supply a context token to each attached child. All tokens are zero
// as context is unused in this project.
U32 rateGroup10HzContext[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup1HzContext[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX]  = {};

enum TopologyConstants {
    CMD_SEQ_BUFFER_SIZE = 5 * 1024,
};

/**
 * \brief configure/setup components in project-specific way
 *
 * Configures components that require project-specific input such as divisor tables and
 * context arrays. Called from setupTopology() between connectComponents() and loadParameters().
 */
void configureTopology() {
    // Rate group driver needs the divisor list to derive 10 Hz and 1 Hz from the 50 Hz base tick
    rateGroupDriver.configure(rateGroupDivisors);

    // Rate groups need context arrays
    rateGroup10Hz.configure(rateGroup10HzContext, FW_NUM_ARRAY_ELEMENTS(rateGroup10HzContext));
    rateGroup1Hz.configure(rateGroup1HzContext, FW_NUM_ARRAY_ELEMENTS(rateGroup1HzContext));

    // Command sequencer needs a buffer for holding command sequence contents
    cmdSeq.allocateBuffer(0, mallocator, CMD_SEQ_BUFFER_SIZE);

    // Open BQ25756E I2C bus (I2C2: P9_19 SCL, P9_20 SDA)
    bool i2cBmsOk = i2cDriverBms.open("/dev/i2c-2");
    FW_ASSERT(i2cBmsOk);

    // Open HC-12 SET pin as GPIO output, default HIGH (transparent mode)
    // GPIO2_4 (global GPIO 68) — P8_10
    Os::File::Status gpioSetSt = gpioDriverSetPin.open(
        "/dev/gpiochip2", 4,
        Drv::LinuxGpioDriver::GPIO_OUTPUT,
        Fw::Logic::HIGH
    );
    FW_ASSERT(gpioSetSt == Os::File::OP_OK, static_cast<FwAssertArgType>(gpioSetSt));

    // Open BQ25756E INT pin as falling-edge interrupt input
    // GPIO2_3 (global GPIO 67) — P8_8
    Os::File::Status gpioIntSt = gpioDriverInt.open(
        "/dev/gpiochip2", 3,
        Drv::LinuxGpioDriver::GPIO_INTERRUPT_FALLING_EDGE
    );
    FW_ASSERT(gpioIntSt == Os::File::OP_OK, static_cast<FwAssertArgType>(gpioIntSt));
}

void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded parameter loading. Function provided by autocoder.
    loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);
    // Open and start the HC-12 UART driver on BeagleBone Black UART1 at 9600 baud
    bool uartOk = uartDriver.open("/dev/ttyO1",
                                  Drv::LinuxUartDriver::BAUD_9600,
                                  Drv::LinuxUartDriver::NO_FLOW,
                                  Drv::LinuxUartDriver::PARITY_NONE,
                                  1024);
    FW_ASSERT(uartOk);
    uartDriver.start(Os::Task::TASK_PRIORITY_DEFAULT,
                     Os::Task::TASK_DEFAULT,
                     Os::Task::TASK_DEFAULT);

    // Start the GPIO interrupt poll thread for the BQ25756E INT pin
    gpioDriverInt.start(
        Os::Task::TASK_PRIORITY_DEFAULT,
        Os::Task::TASK_DEFAULT,
        Os::Task::TASK_DEFAULT
    );
}

void startRateGroups(const Fw::TimeInterval& interval) {
    // The timer component drives the fundamental tick rate of the system.
    // Svc::RateGroupDriver will divide this down to the slower rate groups.
    // This call will block until the stopRateGroups() call is made.
    // For this Linux deployment, that call is made from a signal handler.
    linuxTimer.startTimer(interval);
}

void stopRateGroups() {
    linuxTimer.quit();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);

    // Stop driver threads
    uartDriver.quitReadThread();
    uartDriver.join();
    gpioDriverInt.stop();
    gpioDriverInt.join();

    // Resource deallocation
    cmdSeq.deallocateBuffer(mallocator);

    tearDownComponents(state);
    deinitComponents(state);
}

}  // namespace FeatherCdh
