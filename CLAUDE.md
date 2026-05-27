# feather-cdh — Project Context for Claude Code

## Project Summary

`feather-cdh` is a hardware-in-the-loop (HIL) test environment for a CubeSat EPS (Electrical Power System). The goal is to run the **F Prime flight software framework on FreeRTOS** on a microcontroller that interfaces with two custom PC/104-form-factor power system boards (PDS and BMS), and communicates with a ground station via a 433 MHz radio link.

The project integrates only the **EPS** and **Comms** subsystems into an F Prime deployment. There is no ADCS, no payload, no science inference — just power management, watchdog pinging, and RF telemetry/commanding.

**The project proposal (`.claude/EE 474 Project Proposal Draft.md`) captures the original project vision.** The authoritative design spec is `docs/sdd.md` and the per-component docs under `docs/`.

---

## Target Hardware

### Microcontroller

**Primary:** Adafruit Feather M4 Express (ATSAMD51 Cortex-M4, 120 MHz).

Current development targets desktop Linux (native F Prime build) to unblock software development. FreeRTOS integration is a future step — do not start that migration until the F Prime component development is complete.

### EPS Boards (PC/104 form factor, on hand)

| Board | Purpose | Key ICs |
|-------|---------|---------|
| **PDS** (Power Distribution System) | Steps battery pack down to 12V/5V/3.3V via buck converters; hosts hardware watchdog; provides PC/104 bus | **TI TPS3431SDRBR** window watchdog timer |
| **BMS** (Battery Management System) | Buck-boost MPPT conversion between 6S Li-ion battery pack and solar array | **TI BQ25756E** MPPT solar charge controller |

Both boards connect to the flight computer via the **PC/104 shared bus** (I2C + power rails).

### Battery Pack

6× Samsung INR18650-35E Li-ion cells in 6S configuration (~21.6 V nominal, 3500 mAh per cell).

### Radio (to purchase)

**HC-12 433 MHz UART radio module (×2)** — half-duplex FSK transceiver, 9600 bps default, 3.3–5V compatible. One module on the flight computer side, one on the PC/GDS side connected via USB-TTL adapter.

---

## F Prime Architecture

### Framework

- **F Prime (F')**, `nasa/fprime@devel`
- **OS**: FreeRTOS (via `fprime-freertos` community OSAL package)
- **Scheduling**: 50 Hz base rate group → 10 Hz (watchdog ping, comms drain) → 1 Hz (EPS polling, state machine, health)

### Three-Layer App-Manager-Driver Pattern

```
Layer 3 — Application (*Application)
    EPSApplication       ← power monitoring, command forwarding, powerState cache
    CommsApplication     ← radio management (HC-12 over UART)

Layer 2 — Hardware Managers (*Manager)
    MpptIcManager        ← sole owner of BQ25756E over I2C
    INA3221Manager       ← sole owner of INA3221 over I2C (PDS-dependent)
    WatchdogPinger       ← pulses TPS3431S watchdog GPIO each rate group tick (PDS-dependent)
    HC12Manager          ← manages HC-12 radio; implements Svc.Com (Com Adapter, replaces ComStub)
                           upward and Drv.ByteStreamDriverClient downward toward LinuxUartDriver

Layer 1 — F' Native Bus Drivers (*Driver)
    LinuxI2cDriver (×2)  ← one per I2C device: BQ25756E (0x6B), INA3221 (addr TBD)
    LinuxUartDriver      ← UART to HC-12 radio
    LinuxGpioDriver (×3) ← TPS3431S WDI output, HC-12 SET output, BQ25756E INT input
```

### Pre-built Subtopologies to Include

| Subtopology / Component | Purpose |
|-------------------------|---------|
| `CdhCore` | CmdDispatcher, ActiveLogger (replaces EventManager; ground-commandable severity filter), Health, Version, AssertFatalAdapter, fatalHandler |
| `TlmPacketizer` | Standalone; replaces TlmChan. Packs telemetry channels into configurable downlink packets; individual packets ground-commandable enable/disable for Safe mode bandwidth management. |
| `ComCcsds.FramingSubtopology` | CCSDS framing/deframing pipeline **without** `Svc.ComStub`. The user is responsible for wiring 5 connections to a component implementing `Svc.Com` — `HC12Manager` provides those endpoints (it is the Com Adapter, replacing ComStub). |
| `FileHandling` | PrmDb (parameter persistence); FileUplink/Downlink optional |

**Note:** `DataProducts` and science subtopologies are out of scope.

### Rate Groups

| Rate Group | Frequency | Scheduled Components |
|------------|-----------|---------------------|
| `RateGroup10Hz` | 10 Hz | `WatchdogPinger`, `CommsApplication` |
| `RateGroup1Hz` | 1 Hz | `SatStateMachine`, `EPSApplication`, `MpptIcManager`, `INA3221Manager`, `HC12Manager`, `LinuxUartDriver` (telemetry run), `$health` |

### SatStateMachine

`SatStateMachine` is a Layer 4 Active component with a flat two-state F Prime state machine (Safe / Standby). It is the sole authority on satellite mode. On each 1 Hz tick it calls `EPSApplication.powerStateGet` (sync get) and autonomously transitions to Safe if `vbatt` falls below `CRITICAL_THRESHOLD`. It also accepts two ground commands: `SAFE_MODE` (force Safe from any state) and `SAFE_EXIT` (Safe → Standby). On every mode transition it sends a typed `Comms.Mode` command to `CommsApplication` via `commsModeOut`. The satellite boots into Safe mode.

---

## EPS Component Designs

### EPSApplication (Layer 3, Active)

Runs continuously, no hierarchical state machine. On each 1 Hz tick:
1. Reads cached battery state from `MpptIcManager` (via `batteryStateIn` port)
2. Checks vbatt against `POWER_THRESHOLD` and `CRITICAL_THRESHOLD` parameters
3. Emits `WARNING_HI` events as needed
4. Assembles and caches `powerState` struct

**Commands:**
- `SET_IC_REGISTER(regAddr: BQ25756Reg, value: U32)` — forwards to `MpptIcManager.setRegister` unconditionally

**Ports:** `schedIn` (1 Hz), `batteryStateIn`, `powerStateGet` (guarded input, returns `PowerState` to `SatStateMachine`), `setRegister` (out to `MpptIcManager`), `pingIn/pingOut`, events, prmGet/prmSet.

**No telemetry** — all BQ25756E measurements are already emitted by `MpptIcManager`. EPSApplication's unique outputs are threshold-violation events and `powerStateGet`.

Health-monitored: **yes**.

### MpptIcManager (Layer 2, Active)

Sole owner of BQ25756E over I2C. Standard five-state flat F' SM (RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN). RESET writes `REG_RST=1` (software register reset) in place of a hardware GPIO reset. ENABLE reads `PART_INFO` to verify device ID. CONFIGURE writes watchdog disable, ADC enable, and charging config registers. RUN reads all measurement and status registers each tick, assembles `BatteryState`, and pushes to `EPSApplication`. `intPin` interrupt in RUN → RESET. `setRegister` calls during non-RUN states dropped with `WARNING_LO`.

**Ports:** `schedIn`, `busWrite` (Drv.I2c), `busWriteRead` (Drv.I2cWriteRead), `intPin` (async input, `Svc.Cycle` from `LinuxGpioDriver.gpioInterrupt`), `batteryStateOut`, `setRegister` (async input from EPSApplication), telemetry, events.

Health-monitored: **no** (hardware manager).

### WatchdogPinger (Layer 2, Passive)

Pulses `watchdogPing` GPIO on every `schedIn` tick. Satisfies the TPS3431SDRBR window watchdog. No state machine. If scheduling halts, hardware watchdog expires and resets the system.

**TPS3431S note:** minimum pulse width to reset the timer is 50 ns — no OS delay needed between assert and deassert.

**Ports:** `schedIn`, `watchdogPing` (GpioWrite).

Health-monitored: **no**.

---

## BQ25756E Register Map

**I2C address: `0x6B`**

Register addresses are also documented in `datasheets/bq25756e.pdf`.

### 16-bit Registers (read/write 2 bytes)

| Address | Name | Description |
|---------|------|-------------|
| `0x00` | `CHARGE_VOLT_LIM` | Charge Voltage Limit |
| `0x02` | `CHARGE_CURR_LIM` | Charge Current Limit |
| `0x06` | `IMP_CURR_DPM_LIM` | Input Current DPM Limit |
| `0x08` | `IMP_VOLT_DPM_LIM` | Input Voltage DPM Limit |
| `0x0A` | `REV_IMP_CURR_LIM` | Reverse Mode Input Current Limit |
| `0x0C` | `REV_IMP_VOLT_LIM` | Reverse Mode Input Voltage Limit |
| `0x10` | `PRECHARGE_CURR_LIM` | Precharge Current Limit |
| `0x12` | `TERM_CURR_LIM` | Termination Current Limit |
| `0x1F` | `VAC_MAX_POW_POINT` | VAC Max Power Point Detected |
| `0x2D` | `IAC_ADC` | Input Current ADC reading |
| `0x2F` | `IBAT_ADC` | Battery Current ADC reading |
| `0x31` | `VAC_ADC` | Input Voltage ADC reading |
| `0x33` | `VBAT_ADC` | Battery Voltage ADC reading |
| `0x37` | `TS_ADC` | Temperature Sensor ADC reading |
| `0x39` | `VFB_ADC` | Feedback Voltage ADC reading |

### 8-bit Registers (read/write 1 byte)

| Address | Name | Description |
|---------|------|-------------|
| `0x14` | `PRECHARGE_TERM_CONT` | Precharge and Termination Control |
| `0x15` | `TIME_CONT` | Timer Control |
| `0x16` | `THR_STG_CHARGE_CONT` | Three-Stage Charge Control |
| `0x17` | `CHARGER_CONT` | Charger Control |
| `0x18` | `PIN_CONT_REG` | Pin Control |
| `0x19` | `POW_PATH_REV_CONT` | Power Path and Reverse Mode Control — **bit 7 = `REG_RST`**: writing 1 resets all registers to power-on defaults, self-clears |
| `0x1A` | `MPPT_CONT` | MPPT Control — **bit 0 = `EN_MPPT`**: 0=disable, 1=enable (when enabled, ADC is device-controlled and writes to ADC_CHANNEL_CONT are ignored); bits [2:1] = `FULL_SWEEP_TMR`: 00=3 min, 01=10 min, 10=15 min, 11=20 min |
| `0x1B` | `CHARGE_THRESH_CONT` | Charging Threshold Control |
| `0x1C` | `CHARGE_REGION_CONT` | Charging Region Behavior Control |
| `0x1D` | `REV_THRESH_CONT` | Reverse Mode Threshold Control |
| `0x1E` | `REV_UNDERVOLT_CONT` | Reverse Undervoltage Control |
| `0x21` | `CHARGER_STATUS_1` | Charger Status 1 (read-only) |
| `0x22` | `CHARGER_STATUS_2` | Charger Status 2 (read-only) |
| `0x23` | `CHARGER_STATUS_3` | Charger Status 3 (read-only) |
| `0x24` | `FAULT_STATUS` | Fault Status (read-only) |
| `0x25` | `CHARGER_FLAG_1` | Charger Flag 1 |
| `0x26` | `CHARGER_FLAG_2` | Charger Flag 2 |
| `0x27` | `FAULT_FLAG` | Fault Flag (cleared on read) |
| `0x28` | `CHARGER_MASK_1` | Charger Mask 1 (INT mask) |
| `0x29` | `CHARGER_MASK_2` | Charger Mask 2 (INT mask) |
| `0x2A` | `FAULT_MASK` | Fault Mask (INT mask) |
| `0x2B` | `ADC_CONT` | ADC Control |
| `0x2C` | `ADC_CHANNEL_CONT` | ADC Channel Control |
| `0x3B` | `GATE_DRV_STRG_CONT` | Gate Driver Strength Control |
| `0x3C` | `GATE_DRV_DEATTIME_CONT` | Gate Driver Dead Time Control |
| `0x3D` | `PART_INFO` | Part Information (device ID) |
| `0x62` | `REV_BATT_DISCHARGE_CURR` | Reverse Mode Battery Discharge Current |

**Confirmed ADC scaling:**
- `VBAT_ADC` (0x33): 2 mV/LSB (raw × 2 = millivolts)
- `VAC_ADC` (0x31): 2 mV/LSB
- `VFB_ADC` (0x39): 2 mV/LSB
- `CHARGE_CURR_LIM` (0x02): bits [10:2], 50 mA/LSB
- `IBAT_ADC`, `IAC_ADC`, `TS_ADC` scaling: TBD (confirm from datasheet)
- I2C byte order: big-endian (MSB first) for all 16-bit registers

**CHARGER_STATUS_1 (0x21) bits [2:0] charging state enum:**
0x00=Not Charging, 0x01=Trickle, 0x02=Pre-charge, 0x03=Fast-charge (CC), 0x04=Taper (CV), 0x05=Reserved, 0x06=Top-off Timer, 0x07=Termination Done

**No hardware RESET pin on BQ25756E** — `icReset` GPIO does not exist. Software register reset via `REG_RST` (bit 7 of `POW_PATH_REV_CONT` 0x19): writing 1 resets all registers to power-on defaults, bit self-clears. `MpptIcManager` writes `REG_RST=1` in the `RESET` state before the init sequence proceeds. `WD_RST` (CHARGER_CONT bit 5) is distinct — it only resets the internal I2C watchdog timer.

**Key registers for MpptIcManager telemetry polling (each tick):**
- Read: `VBAT_ADC` (0x33), `IBAT_ADC` (0x2F), `VAC_ADC` (0x31), `IAC_ADC` (0x2D), `TS_ADC` (0x37), `VFB_ADC` (0x39)
- Read: `CHARGER_STATUS_1/2/3` (0x21–0x23), `FAULT_STATUS` (0x24), `FAULT_FLAG` (0x27)

**Key registers for initialization:**
- `ADC_CONT` (0x2B): enable ADC, select continuous mode
- `ADC_CHANNEL_CONT` (0x2C): enable all ADC channels
- `TIME_CONT` (0x15): disable internal watchdog (prevents charger settings reset)
- `CHARGER_CONT` (0x17): charging control; bits [7:6] = Vrechg threshold
- `MPPT_CONT` (0x1A): MPPT mode configuration
- `CHARGE_VOLT_LIM` (0x00): target charge voltage (formula: mV = raw × 2 + 1504)
- `CHARGE_CURR_LIM` (0x02): max charge current (bits [10:2], 50 mA/LSB)
- `TERM_CURR_LIM` (0x12): termination threshold
- `PART_INFO` (0x3D): read at startup to verify device ID

---

## HC-12 Radio Hardware Facts

Source: HC-12 V2.4 User Manual (2016-12-02).

| Property | Value |
|----------|-------|
| SET pin logic | **LOW = AT command mode**; HIGH or float = transparent mode (internal 10 kΩ pull-up) |
| UART levels | 3.3 V TTL; internal 1 kΩ series resistors on RXD and TXD |
| Supply | 3.2 V – 5.5 V DC; ≥ 200 mA |
| Factory defaults | FU3 mode, 9600 bps (8N1), CH001 = 433.4 MHz, +20 dBm |
| AT entry timing | Pull SET LOW → wait **40 ms** → send AT command |
| AT exit timing | Assert SET HIGH → wait **80 ms** → transparent mode ready |
| Re-entry guard | After exiting AT mode, wait **≥ 200 ms** before re-entering |
| RF duplex | Half-duplex; handled internally by module |

**AT commands for HC12Manager CONFIGURE state:**
- `AT` → `OK` (verify AT mode active)
- `AT+B9600` → set baud rate
- `AT+C001` → set channel (433.4 MHz)
- `AT+FU3` → FU3 full-speed mode
- `AT+P8` → max power (+20 dBm)
- `AT+RX` → read all parameters (use to verify current settings)

Settings are **persistent across power cycles** — factory defaults are acceptable for this project, so CONFIGURE may just verify via `AT+RX` and skip redundant writes.

## Comms Design

The radio link uses an **HC-12 433 MHz UART module** as a transparent byte pipe.

- The HC-12 is a transparent UART link — bytes sent to the UART appear at the remote end, and vice versa.
- `HC12Manager` implements `Svc.Com` (the Com Adapter interface) upward into the `ComCcsds.FramingSubtopology` — it replaces `Svc.ComStub`. Downward it implements `Drv.ByteStreamDriverClient` to talk to `LinuxUartDriver`.
- `CommsApplication` controls the `ComCcsds` ComQueue drain rate based on satellite mode.
- The F Prime GDS on the ground side connects via USB-TTL to the second HC-12 module.

---

## Repository Structure

```
feather-cdh/
├── CMakeLists.txt
├── CMakePresets.json
├── settings.ini
├── CLAUDE.md
├── datasheets/
│   ├── bq25756e.pdf
│   ├── HC-12_english_datasheets.pdf
│   └── ina3221.pdf
├── docs/
│   ├── sdd.md                          ← top-level system design document
│   ├── Core/
│   │   └── SatStateMachine.md
│   ├── EPS/
│   │   ├── EPSApplication.md
│   │   ├── MpptIcManager.md
│   │   ├── INA3221Manager.md
│   │   └── WatchdogPinger.md
│   └── Comms/
│       ├── CommsApplication.md
│       └── HC12Manager.md
└── FeatherCdh/
    ├── CMakeLists.txt
    ├── Types/
    │   ├── CMakeLists.txt
    │   └── Types.fpp                   ← Sat.Mode, Comms.Mode, BQ25756Reg, BatteryState, PowerState, custom ports
    └── Components/
        ├── CMakeLists.txt
        ├── SatStateMachine/
        │   └── SatStateMachine.fpp
        ├── EPSApplication/
        │   └── EPSApplication.fpp
        ├── MpptIcManager/
        │   └── MpptIcManager.fpp
        ├── INA3221Manager/
        │   └── INA3221Manager.fpp
        ├── WatchdogPinger/
        │   └── WatchdogPinger.fpp
        ├── CommsApplication/
        │   └── CommsApplication.fpp
        └── HC12Manager/
            └── HC12Manager.fpp
```

---

## Testing Conventions (STest / F Prime Structured Tests)

All component unit tests use the **F Prime STest framework** (`STest/` in the fprime repo), not plain GTest. STest adds rule-based and scenario-based testing on top of GTest.

### File layout

Each component's tests live in `Components/<Name>/test/ut/` with **five** files:

| File | Purpose |
|------|---------|
| `<Name>Tester.hpp` | Defines `<Name>Tester`, which extends `<Name>GTestBase`. Holds the component under test, `TEST_INSTANCE_ID`, `MAX_HISTORY_SIZE`, stub return values, the constructor, and output port handler overrides. Declares `connectPorts()` and `initComponents()` — bodies are auto-generated. |
| `TestState.hpp` | Declares `<Name>TestState`, which extends `<Name>Tester`. Only adds `precondition__GROUP__RULE` / `action__GROUP__RULE` method declarations. |
| `TestState.cpp` | Implements all rule method bodies. No constructor, no port wiring — those live in `<Name>Tester`. |
| `Rules.hpp` | Uses a macro to expand each `(GROUP, RULE)` pair into an `STest::Rule<<Name>TestState>` struct that delegates to the correspondingly named methods on the state. |
| `TestMain.cpp` | GTest `TEST(...)` cases — typically one direct single-rule invocation and one `RandomScenario` + `BoundedScenario` run. Also provides `main()` with `STest::Random::seed()`. |

The `register_fprime_ut()` call lives in the component's own `CMakeLists.txt` (not a separate file in `test/ut/`) and must include `UT_AUTO_HELPERS`. This flag triggers autocoding of `<Name>TesterHelpers.cpp` in the build directory, which provides the bodies of `connectPorts()` and `initComponents()` for `<Name>Tester`.

### Three-layer class hierarchy

```
<Name>GTestBase          ← autocoded; provides ASSERT_from_* macros, port history
    └── <Name>Tester     ← developer-written; owns component, port handlers, init
        └── <Name>TestState  ← developer-written; STest rule methods only
```

### `<Name>Tester.hpp` template

```cpp
#pragma once
#include "<Name>GTestBase.hpp"
#include "FeatherCdh/Components/<Name>/<Name>.hpp"

namespace FeatherCdh {

class <Name>Tester : public <Name>GTestBase {
  public:
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;
    static constexpr U32 MAX_HISTORY_SIZE = 16;

    <Name> component;
    // inject stub return values here, e.g.: Drv::GpioStatus m_gpioStatus = Drv::GpioStatus::OP_OK;

    <Name>Tester()
        : <Name>GTestBase("<Name>Tester", MAX_HISTORY_SIZE),
          component("<Name>") {
        this->initComponents();   // calls this->init() then component.init()
        this->connectPorts();     // wires all ports
        this->clearHistory();
    }

    void connectPorts();    // body provided by UT_AUTO_HELPERS
    void initComponents();  // body provided by UT_AUTO_HELPERS

  protected:
    // Override output port handlers to record calls and return stub values.
    // Signatures must match the autocoded virtual exactly — use const Fw::X& not Fw::X.
    ReturnType from_<portName>_handler(FwIndexType portNum, const ArgType& arg) override {
        this->pushFromPortEntry_<portName>(arg);
        return this->m_stubValue;
    }
};

}  // namespace FeatherCdh
```

### Critical: output port handler signatures

The autocoded `<Name>TesterBase` declares `from_<portName>_handler` with **const reference** parameters (e.g., `const Fw::Logic& state`). Override declarations must match exactly — using pass-by-value instead of const reference will silently fail to override and the compiler will reject `override`. Always check the generated `<Name>TesterBase.hpp` in the build directory for the exact signature.

### `connectPorts()` and `initComponents()` — do not implement manually

`UT_AUTO_HELPERS` auto-generates these two methods in `<Name>TesterHelpers.cpp` in the build directory. The generated `initComponents()` calls `this->init()` (which wires the tester's from-ports with `addCallComp`) before `component.init()`. If `this->init()` is never called, from-port `m_comp` stays null and any output port invocation asserts at runtime inside the port's `invoke()`. Never re-implement these methods in `TestState.cpp`.

### Rule naming convention

Rules are named `GROUP::RULE` and map to `precondition__GROUP__RULE` / `action__GROUP__RULE` on the state. Example groups: `Ping`, `Init`, `Run`, `Fault`. Example rules within a group: `Nominal`, `GpioError`, `I2cError`, `Timeout`.

### Key STest types

- `STest::Rule<State>` — base for a rule; override `precondition(const State&)` and `action(State&)`.
- `STest::RandomScenario<State>` — draws applicable rules at random each step.
- `STest::BoundedScenario<State>` — wraps a scenario and stops after N steps.
- `STest::Random::seed()` — seeds the PRNG (call once in `main`).

### Active vs Passive components

- **Passive** (sync ports): `invoke_to_<port>` executes the handler immediately; assertions follow directly.
- **Active** (async queued ports): `invoke_to_<port>` enqueues the message. Do NOT call `component.doDispatch()` directly — it is **private** on Active components. Use the static helpers provided by the autocoded TesterBase instead:
  - `EPSApplicationTesterBase::dispatchOne(component)` — drains one message
  - `EPSApplicationTesterBase::dispatchCurrentMessages(component)` — drains all queued messages
  The class name prefix matches the component under test (e.g. `EPSApplicationTesterBase::dispatchOne`).

### Protected opcode constants

`OPCODE_<CMD>` constants are **protected** on the component base class and inaccessible from the tester (which does not inherit from the component). Use the raw numeric value from the autocoded `.hpp` instead (e.g. `OPCODE_SET_IC_REGISTER = 0x0` → use `0x0` in `ASSERT_CMD_RESPONSE`).

### Auto-helpers constant naming

The autocoded `initComponents()` expects `TEST_INSTANCE_QUEUE_DEPTH` (for Active components) and `TEST_INSTANCE_ID` as static constants on the Tester class. Use exactly these names — a mismatch causes a compile error.

### Running tests

```bash
# One-time setup (or after any CMakeLists change):
fprime-util purge
fprime-util generate --ut

# Build and run a single component's tests:
cd FeatherCdh/Components/<Name>
fprime-util build --ut
fprime-util check          # note: no --ut flag on check

# Or build/run all UTs from the project root:
fprime-util build --ut
fprime-util check
```

### CMakeLists stub for incomplete components

If a component's `register_fprime_ut()` references test source files that don't exist yet, `generate --ut` will fail for the entire project. Either comment out that `register_fprime_ut()` block until the files are written, or comment out the component's `add_fprime_subdirectory` in the parent `CMakeLists.txt` to exclude it entirely from the UT build.

---

## Key Design Decisions and Constraints

1. **SatStateMachine has Safe and Standby modes only.** Transitions: ground commands (`SAFE_MODE`/`SAFE_EXIT`) or vbatt below `CRITICAL_THRESHOLD`.
2. **EPSApplication has no mode port.** It runs identically regardless of satellite mode.
3. **Application components do not know the satellite's mode representation.** They receive their own subsystem mode enum from SatStateMachine. They must not import or reference `Sat::Mode` or `Sat::StandbySubmode`.
4. **CommsApplication controls the ComQueue drain rate.** It is the sole driver of `comQueue.run`. In `Safe` mode the drain fires every `SAFE_DRAIN_DIVISOR` ticks (default 1 Hz); in `Normal` mode it fires each 10 Hz tick. Event severity filtering (ActiveLogger) and telemetry packet filtering (TlmPacketizer) are separately configurable via ground command.
5. **No DeployPanelsManager.** The burn wire is not on the PDS board. `DEPLOY_PANELS` command does not exist.
6. **Two deployment topologies:** `FeatherCdhFullDeployment` (all hardware including PDS) and `FeatherCdhPartialDeployment` (BMS + radio only, no PDS components). PDS-dependent components (`WatchdogPinger`, `INA3221Manager`, their drivers) are excluded from the partial deployment — not stubbed out, just absent.
7. **BQ25756E interface is plain I2C.** `MpptIcManager` uses direct I2C read/write calls — no complex class hierarchy.
8. **HC-12 is the radio link.** `HC12Manager` is the Com Adapter: it implements `Svc.Com` upward (replacing `Svc.ComStub`) and `Drv.ByteStreamDriverClient` downward toward `LinuxUartDriver`. The project uses `ComCcsds.FramingSubtopology` (no ComStub instance).
9. **`BQ25756Reg` enum** (register names for `SET_IC_REGISTER` command) should be defined to match the register table above, with human-readable names replacing raw hex addresses.
10. **WatchdogPinger runs at 10 Hz.** TPS3431S minimum WDI pulse width is 50 ns — assert then immediately deassert with no delay needed. PDS window watchdog must be configured for a compatible window around 10 Hz.
