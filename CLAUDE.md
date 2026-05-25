# feather-cdh — Project Context for Claude Code

## Project Summary

`feather-cdh` is a hardware-in-the-loop (HIL) test environment for a CubeSat EPS (Electrical Power System). The goal is to run the **F Prime flight software framework on FreeRTOS** on a microcontroller that interfaces with two custom PC/104-form-factor power system boards (PDS and BMS), and communicates with a ground station via a 433 MHz radio link.

The project is a **targeted subset of the full HS2 3U CubeSat design**: it integrates only the **EPS** and **Comms** subsystems into an F Prime deployment. There is no ADCS, no payload, no science inference — just power management, watchdog pinging, and RF telemetry/commanding.

**The ESP-32 project proposal (`.claude/EE 474 Project Proposal Draft.md`) fully captures the spirit of the project.** The HS2 SDD describes the broader architecture this project draws from, but is not the authoritative spec — adapt the design for the constraints below.

---

## Target Hardware

### Microcontroller

**Primary:** Adafruit Feather M4 Express (ATSAMD51 Cortex-M4, 120 MHz). Reference F Prime + FreeRTOS port exists at `fprime-featherm4-freertos-reference`.

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

**HC-12 433 MHz UART radio module (×2)** — half-duplex FSK transceiver, 9600 bps default, 3.3–5V compatible. One module on the flight computer side, one on the PC/GDS side connected via USB-TTL adapter. This replaces the EnduroSat S-band radio used in full HS2.

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
    CommsApplication     ← radio management (simplified: HC-12 over UART instead of EnduroSat S-band)

Layer 2 — Hardware Managers (*Manager)
    MpptIcManager        ← sole owner of BQ25756E over I2C
    INA3221Manager       ← sole owner of INA3221 over I2C (PDS-dependent)
    WatchdogPinger       ← pulses TPS3431S watchdog GPIO each rate group tick (PDS-dependent)
    HC12Manager          ← manages HC-12 radio; implements Drv::ByteStreamDriver toward ComCcsds

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
| `ComCcsds` | CCSDS framing/deframing pipeline; connects to HC-12 UART driver |
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

## EPS Component Designs (from HS2 SDD)

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

Source: `HS2-EPS-flightSW/HS2-EPS-BatteryManager/include/BQ25756_reg.h`. The HS2-EPS-flightSW repo is an older Arduino-based implementation for reference only — do not copy its driver structure. Use it only for register addresses.

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

**Confirmed ADC scaling (from HS2-EPS-flightSW reference code):**
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

## Comms Design (adapted from HS2)

The HS2 design uses an EnduroSat S-band radio managed by `EnduroSatManager`. For this project, the radio is replaced by the **HC-12 433 MHz UART module**.

- The HC-12 is a transparent UART link — bytes sent to the UART appear at the remote end, and vice versa.
- `CommsApplication` in this project is simplified: it manages the HC-12 UART driver and bridges the F Prime `ComCcsds` communications stack.
- A `UartRadioManager` (working name) replaces `EnduroSatManager` — it wraps a `LinuxUartDriver`/equivalent FreeRTOS UART driver and implements `Drv::ByteStreamDriverModel`.
- The F Prime GDS on the ground side connects via USB-TTL to the second HC-12 module.

---

## Reference Repositories

| Repo | Path | Purpose |
|------|------|---------|
| `hs2-software-design` | `~/hs2-software-design` | Full HS2 SDD and component docs — architectural reference |
| `HS2-EPS-flightSW` | `~/HS2-EPS-flightSW` | Older Arduino-based BQ25756 driver — **use only for register addresses**, not driver structure |
| `fprime-featherm4-freertos-reference` | `~/fprime-featherm4-freertos-reference` | Complete F Prime + FreeRTOS port for Feather M4 fallback — primary platform reference |
| `fprime-workshop-led-blinker` | `~/fprime-workshop-led-blinker` | F Prime workshop example — useful for topology/component boilerplate reference |

---

## Key Design Decisions and Constraints

1. **SatStateMachine has Safe and Standby modes only.** Transitions: ground commands (`SAFE_MODE`/`SAFE_EXIT`) or vbatt below `CRITICAL_THRESHOLD`.
2. **EPSApplication has no mode port.** It runs identically regardless of satellite mode.
3. **Application components do not know the satellite's mode representation.** They receive their own subsystem mode enum from SatStateMachine. They must not import or reference `Sat::Mode` or `Sat::StandbySubmode`.
4. **CommsApplication controls the ComQueue drain rate.** It is the sole driver of `comQueue.run`. In `Safe` mode the drain fires every `SAFE_DRAIN_DIVISOR` ticks (default 1 Hz); in `Normal` mode it fires each 10 Hz tick. Event severity filtering (ActiveLogger) and telemetry packet filtering (TlmPacketizer) are separately configurable via ground command.
5. **No DeployPanelsManager.** The burn wire is not on the PDS board. `DEPLOY_PANELS` command does not exist.
6. **Two deployment topologies:** `FeatherCdhFullDeployment` (all hardware including PDS) and `FeatherCdhPartialDeployment` (BMS + radio only, no PDS components). PDS-dependent components (`WatchdogPinger`, `INA3221Manager`, their drivers) are excluded from the partial deployment — not stubbed out, just absent.
7. **HS2-EPS-flightSW is not the driver design to follow.** It is bloated Arduino code. The BQ25756E interface in this project should be plain I2C read/write calls from `MpptIcManager` — no complex class hierarchy.
8. **HC-12 replaces EnduroSat S-band.** `HC12Manager` implements `Drv::ByteStreamDriverModel` and wraps `LinuxUartDriver`. Same `ComCcsds` subtopology applies.
9. **`BQ25756Reg` enum** (register names for `SET_IC_REGISTER` command) should be defined to match the register table above, with human-readable names replacing raw hex addresses.
10. **WatchdogPinger runs at 10 Hz.** TPS3431S minimum WDI pulse width is 50 ns — assert then immediately deassert with no delay needed. PDS window watchdog must be configured for a compatible window around 10 Hz.
