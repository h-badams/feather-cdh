# FeatherCdh Flight Software — Software Design Document

**Date:** 2026-05-25
**Framework:** F Prime (`nasa/fprime@devel`)
**Platform:** Adafruit Feather M4 Express (ATSAMD51 Cortex-M4, 120 MHz)

---

## 1. Project Overview

FeatherCdh is a hardware-in-the-loop (HIL) test deployment for a CubeSat Electrical Power System (EPS). It runs the F Prime flight software framework on an Adafruit Feather M4, interfacing with two custom PC/104-form-factor power system boards and communicating with an F Prime Ground Data System (GDS) via a 433 MHz HC-12 radio link.

The project is a targeted subset of the full HS2 3U CubeSat design. It integrates the **EPS** and **Comms** subsystems only. There is no ADCS, no science payload, and no data product pipeline. The primary goal is to validate BMS hardware behavior (BQ25756E MPPT charge controller) under real flight software and exercise the full command/telemetry path from GDS through the radio to the flight computer.

---

## 2. Hardware Inventory

| Hardware | Board | Interface | Primary Software Owner |
|----------|-------|-----------|----------------------|
| TI BQ25756E MPPT Solar Charge Controller | BMS | I2C | `MpptIcManager` |
| TI INA3221 Triple-Channel Current/Voltage Monitor | PDS | I2C | `INA3221Manager` *(PDS-dependent)* |
| TI TPS3431SDRBR Window Watchdog Timer | PDS | GPIO | `WatchdogPinger` *(PDS-dependent)* |
| HC-12 433 MHz UART Radio Module | — | UART | `HC12Manager` |

**PDS-dependent hardware** is only present in `FeatherCdhFullDeployment`. The BMS is confirmed manufactured and is the primary hardware under test.

---

## 3. Operational Modes

Two modes managed by `SatStateMachine`. The satellite boots into **Safe** mode.

| Mode | Description |
|------|-------------|
| **Safe** | Initial and emergency mode. EPS monitoring always active. Comms in `Safe` sub-mode: ComQueue drains at reduced rate (`SAFE_DRAIN_DIVISOR` parameter, default 1 Hz); event severity and telemetry packet filtering configurable via ground command. |
| **Standby** | Normal operations. EPS monitoring always active. Comms in `Normal` sub-mode: ComQueue drains actively each rate group tick. |

### Mode Transitions

| From | To | Trigger |
|------|----|---------|
| Any | Safe | Ground command `SAFE_MODE` |
| Any | Safe | `vbatt` below `CRITICAL_THRESHOLD` parameter (evaluated each 1 Hz tick by `SatStateMachine`) |
| Safe | Standby | Ground command `SAFE_EXIT` |

---

## 4. Architecture Overview

The flight software uses a **five-layer architecture**. Component names reflect their layer.

```
Layer 5 — System Infrastructure
    CdhCore (CmdDispatcher, ActiveLogger, Health, Version, AssertFatalAdapter, fatalHandler)
    TlmPacketizer

Layer 4 — Mission Orchestration
    SatStateMachine

Layer 3 — Application Components (*Application) + Pre-built Subtopologies
    EPSApplication | CommsApplication
    + ComCcsds subtopology | FileHandling subtopology

Layer 2 — Hardware Managers (*Manager / *Pinger)
    MpptIcManager | WatchdogPinger | INA3221Manager | HC12Manager

Layer 1 — F' Native Bus Drivers (*Driver)
    LinuxI2cDriver (×2) | LinuxUartDriver | LinuxGpioDriver (×3)
```

**Layer 5** provides satellite-wide services: command routing (`CmdDispatcher`), event logging with severity filtering and FATAL escalation (`ActiveLogger → fatalHandler`), telemetry packaging (`TlmPacketizer`), component liveness monitoring (`Health`), and version reporting. All other layers depend on Layer 5 services.

**Layer 4** (`SatStateMachine`) evaluates mode conditions each 1 Hz tick and sends typed mode commands to Layer 3 application components that accept a mode port. It has no hardware knowledge.

**Layer 3** application components contain mission logic and dispatch work to hardware managers. They receive mode commands from `SatStateMachine` via typed ports carrying their own mode enum — they do not know the satellite's internal `Safe`/`Standby` representation, only their subsystem's mode state. `EPSApplication` is the exception: it has no mode port and runs continuously regardless of satellite mode.

**Layer 2** hardware managers own individual hardware devices and implement a flat F' state machine following the `RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN` pattern. They have no satellite mode awareness. The exception is `MpptIcManager`, which uses a simplified two-state machine (see §5.3).

**Layer 1** are passive bus drivers with no device knowledge.

---

## 5. Subtopology Decomposition

### 5.1 Layer 5 — CdhCore Subtopology + Infrastructure

Pre-built subtopology plus two standalone infrastructure components. Provides command routing, event collection/filtering/downlink, telemetry packaging, health monitoring, and FATAL-triggered system reset.

| Component | Purpose |
|-----------|---------|
| `cmdDisp` (Svc.CmdDispatcher) | Routes uplink commands to all registered components |
| `events` (Svc.ActiveLogger) | Collects and downlinks events; routes FATAL via `fatalHandler`; ground-commandable severity filter (`SET_EVENT_FILTER`) for bandwidth management in `Safe` mode |
| `$health` (Svc.Health) | Ping-based liveness monitoring |
| `fatalHandler` (Svc.FatalHandler) | Resets system on FATAL event; reboots into Safe mode |
| `fatalAdapter` (Svc.AssertFatalAdapter) | Converts C++ assert failures to FATAL events |
| `version` (Svc.Version) | Reports software version as telemetry |
| `tlmPacketizer` (Svc.TlmPacketizer) | Collects component telemetry and packs it into configurable downlink packets; replaces `TlmChan`. Individual packets can be enabled/disabled via ground command (`SEND_PKT`/`CLR_PKT`) for telemetry bandwidth management. Packet definitions TBD. |

### 5.2 Layer 3 Pre-Built Subtopologies

| Subtopology | Source | Purpose |
|-------------|--------|---------|
| `ComCcsds.FramingSubtopology` | `Svc/Subtopologies/ComCcsds` | CCSDS Space Packet + TM/TC framing pipeline **without** `ComStub`. `HC12Manager` provides the `Svc.Com` (Com Adapter) interface that the framer and frameAccumulator wire into. ComQueue drain is driven by `CommsApplication`, not a rate group directly. |
| `FileHandling` | `Svc/Subtopologies/FileHandling` | `PrmDb` (parameter persistence), `FileUplink`, `FileDownlink`, `FileManager` |

### 5.3 EPS Subtopology

**Purpose:** Monitors BMS battery and power system health, accepts power configuration commands from the ground, and publishes power state to `SatStateMachine` for mode transition decisions. Operates continuously, independent of satellite mode.

| Component | Type | Purpose |
|-----------|------|---------|
| `EPSApplication` | Active | Command-driven orchestrator. Reads battery state from `MpptIcManager` each tick; exposes `powerStateGet` synchronous get to `SatStateMachine`; forwards `SET_IC_REGISTER` to `MpptIcManager`. |
| `MpptIcManager` | Active (worker) | Sole owner of BQ25756E over I2C. Five-state SM (RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN). Reads all measurements each tick; handles INT fault interrupt. |
| `WatchdogPinger` | Passive | Pulses TPS3431SDRBR watchdog GPIO pin each 10 Hz rate group tick. **(PDS-dependent — absent in `FeatherCdhPartialDeployment`)** |
| `INA3221Manager` | Queued (worker) | Reads all three PDS output rail voltages and currents (12 V, 5 V, 3.3 V) over I2C; emits telemetry only. Stub — does not feed `EPSApplication`. **(PDS-dependent — absent in `FeatherCdhPartialDeployment`)** |

`SatStateMachine` retrieves the latest power state from `EPSApplication` each 1 Hz tick by invoking the `powerStateGet` synchronous get port. The returned struct drives Safe/Standby mode transition decisions.

**Health monitoring:** `EPSApplication` is health-monitored. Hardware managers excluded.

### 5.4 Comms Subtopology

**Purpose:** Manages the HC-12 radio link and controls autonomous downlink behavior based on mode commands from `SatStateMachine`.

| Component | Type | Purpose |
|-----------|------|---------|
| `CommsApplication` | Active | Flat SM (Safe / Normal). Receives mode from `SatStateMachine`; controls ComQueue drain rate: every tick in Normal mode, every `SAFE_DRAIN_DIVISOR` ticks in Safe mode. |
| `HC12Manager` | Queued (worker) | Implements `Svc.Com` (Com Adapter) upward into the `ComCcsds.FramingSubtopology` (replaces `Svc.ComStub`) and `Drv.ByteStreamDriverClient` downward to `LinuxUartDriver`. Drives the HC-12 SET pin and sends AT initialization commands during CONFIGURE state; transparent byte passthrough in RUN. |

**Health monitoring:** `CommsApplication` is health-monitored. `HC12Manager` excluded.

---

## 6. Top-Level Standalone Components

| Component | Layer | Type | Purpose |
|-----------|-------|------|---------|
| `SatStateMachine` | 4 | Active | Mission mode orchestration (see §7) |
| `RateGroupDriver` | — | Passive | Divides hardware timer interrupt into rate signals |
| `RateGroup10Hz` | — | Active | 10 Hz scheduling |
| `RateGroup1Hz` | — | Active | 1 Hz scheduling |
| `LinuxI2cDriver` (×2) | 1 | Passive | Both instances open the same physical I2C bus (`/dev/i2c-N`); addresses are per-transaction args. F Prime `guarded input` ports cannot be shared across components, so one instance per component: `MpptIcManager` (BQ25756E, 0x6B) and `INA3221Manager` (addr TBD). |
| `LinuxUartDriver` | 1 | Passive | HC-12 radio UART. Has `run: Svc.Sched` port for telemetry — wired to `RateGroup1Hz`. |
| `LinuxGpioDriver` (×3) | 1 | Passive | One instance per GPIO pin: (1) TPS3431S WDI output → `WatchdogPinger.watchdogPing`; (2) HC-12 SET output → `HC12Manager.setPin`; (3) BQ25756E INT input → `MpptIcManager.intPin` (fires `gpioInterrupt: Svc.Cycle`). |

### Rate Group Schedule

| Rate Group | Frequency | Scheduled Components |
|------------|-----------|---------------------|
| `RateGroup10Hz` | 10 Hz | `WatchdogPinger`, `CommsApplication` |
| `RateGroup1Hz` | 1 Hz | `SatStateMachine`, `EPSApplication`, `MpptIcManager`, `INA3221Manager`, `HC12Manager`, `$health` |

> Note: `ComCcsds` ComQueue is not connected to a rate group directly. It is driven by `CommsApplication` via the `comQueueRun` port: every 10 Hz tick in `Normal` mode, every `SAFE_DRAIN_DIVISOR` ticks in `Safe` mode (default 1 Hz). Running `CommsApplication` at 10 Hz matches the F Prime reference pattern of connecting ComQueue to the fastest rate group. See §5.4 and the `CommsApplication` SDD.
>
> Note: F Prime's `Svc::Health` ping timeout is specified as a number of `schedIn` calls. At 1 Hz, a timeout count of 3 gives a 3-second wall-clock deadline — a reasonable value for embedded component liveness checks. This matches the F Prime reference pattern of placing Health on the slowest rate group.

---

## 7. SatStateMachine Design

`SatStateMachine` is an Active component. It evaluates power conditions and responds to ground commands each 1 Hz tick, then sends typed mode commands to application components.

### Mode Output Ports

One typed output port per application component that accepts a mode:

```fpp
module FeatherCdh {
    port CommsModePort(mode: Comms.Mode)
}
```

`SatStateMachine` owns the translation table from satellite mode to each application's mode enum. Application components receive only their own mode enum; they have no knowledge of the satellite's `Safe`/`Standby` representation.

### Condition Inputs

| Port | Direction | Source | Data |
|------|-----------|--------|------|
| `powerStateGet` | Output (sync get into `EPSApplication`) | `EPSApplication` | Latest `PowerState` struct (vbatt, fault flags, charging status) |

### Translation Table

| Satellite Mode | `CommsApplication` |
|---------------|-------------------|
| Safe | `Comms.Mode.Safe` |
| Standby | `Comms.Mode.Normal` |

### Mode Transitions

| From | To | Trigger |
|------|----|---------|
| Any | Safe | Ground command `SAFE_MODE` |
| Any | Safe | `vbatt` below `CRITICAL_THRESHOLD` (from `powerStateGet`, evaluated each 1 Hz tick) |
| Safe | Standby | Ground command `SAFE_EXIT` |

Events are emitted on every transition. `SatStateMachine` is health-monitored.

---

## 8. Application Component State Machine Pattern

All Layer 3 application components (except `EPSApplication`) use hierarchical F' state machines (`Fw::Sm`) where:

- **Mode is the top-level state** — each mode received from `SatStateMachine` is a parent state
- **Operational substates are nested inside** each mode parent
- **A single `switchMode` signal** is defined at the top level and inherited by all leaf states
- **Mode re-entry always starts from its `initial` substate** — no history

`EPSApplication` does not follow this pattern. It has no mode port and no state machine. It operates continuously, driven only by rate group ticks and commands.

---

## 9. Hardware Manager State Machine Pattern

All hardware managers use a single flat F' state machine:

```
RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN
  ↑_______________ error from any state _______________|
```

- Driven by rate group tick (`schedIn`)
- On error: log `WARNING_HI` (throttled), send `error` signal → back to `RESET` (self-healing)
- Configuration via F' parameters where applicable

---

## 10. Key Cross-Subtopology Wiring

| Source | Destination | Data |
|--------|-------------|------|
| `MpptIcManager.batteryStateOut` | `EPSApplication.batteryStateIn` | BQ25756E measurements and status (vbatt_mV, ibatt_raw, vac_mV, iac_raw, vfb_mV, ts_raw, chargingState, chargerStatus2/3, faultStatus, faultFlag, mppt_enabled) |
| `EPSApplication.setRegister` | `MpptIcManager.setRegister` | Register write forwarded from `SET_IC_REGISTER` ground command (`BQ25756Reg`, `U32`) |
| `EPSApplication.powerStateGet` (sync get) | `SatStateMachine` | Cached `PowerState` struct (pulled by `SatStateMachine` each 1 Hz tick) |
| `SatStateMachine.commsModeOut` | `CommsApplication.commsModeIn` | Mode command (`Comms.Mode`) |
| `CommsApplication.comQueueRun` | `ComCcsds.comQueue.run` | ComQueue drain trigger (fired only in `Comms.Mode.Normal`) |
| `ComCcsds.framer.dataOut` | `HC12Manager.dataIn` | Downlink: framed packet to be transmitted (Com Adapter input) |
| `HC12Manager.dataReturnOut` | `ComCcsds.framer.dataReturnIn` | Downlink buffer ownership return after transmission |
| `HC12Manager.comStatusOut` | `ComCcsds.framer.comStatusIn` | Tx status / ready signal — `SUCCESS` on RUN entry signals framer to start downlink |
| `ComCcsds.frameAccumulator.dataReturnOut` | `HC12Manager.dataReturnIn` | Uplink buffer ownership return |
| `HC12Manager.dataOut` | `ComCcsds.frameAccumulator.dataIn` | Uplink: bytes received from UART, forwarded to frameAccumulator for CCSDS deframing |
| `HC12Manager.drvSendOut` | `LinuxUartDriver.$send` | Downlink: bytes pushed to UART driver |
| `LinuxUartDriver.$recv` | `HC12Manager.drvReceiveIn` | Uplink: bytes received from UART |
| `LinuxUartDriver.ready` | `HC12Manager.drvConnected` | Informational ready signal from UART driver |
| `HC12Manager.drvReceiveReturnOut` | `LinuxUartDriver.recvReturnIn` | Returns receive buffer ownership to UART driver |

---

## 11. Health Monitoring Summary

| Component | Subtopology |
|-----------|-------------|
| `SatStateMachine` | Top-level |
| `EPSApplication` | EPS |
| `CommsApplication` | Comms |
| `cmdDisp` | CdhCore |
| `events` | CdhCore |

All hardware managers (`MpptIcManager`, `WatchdogPinger`, `INA3221Manager`, `HC12Manager`) are excluded from health monitoring.

---

## 12. Deployment Variants

Two deployment configurations exist. They share all component source code; the difference is which components and drivers are wired into the topology.

| Component / Driver | `FeatherCdhFullDeployment` | `FeatherCdhPartialDeployment` |
|-------------------|---------------------------|-------------------------------|
| `WatchdogPinger` | Included | **Excluded** |
| `INA3221Manager` | Included | **Excluded** |
| `LinuxGpioDriver` (watchdog WDI) | Included | **Excluded** |
| `LinuxI2cDriver` (INA3221 instance) | Included | **Excluded** |
| All other components | Included | Included |

`FeatherCdhPartialDeployment` is used when PDS hardware is unavailable. All BMS-interfacing components (`MpptIcManager`, `EPSApplication`) and the radio stack are present in both deployments.
