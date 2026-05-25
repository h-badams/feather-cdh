# SatStateMachine SDD

## 1. Overview

`SatStateMachine` is the Layer 4 Active component. It is the sole authority on the satellite's operational mode. On each 1 Hz rate group tick it checks power conditions by invoking `EPSApplication`'s `powerStateGet` synchronous get port, evaluates any pending mode transition, and sends typed mode commands to all Layer 3 application components that accept a mode port. It also accepts ground commands to force mode transitions.

The satellite boots into **Safe** mode.

---

## 2. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-SSM-001 | `SatStateMachine` shall boot into Safe mode. | Inspection |
| FCD-SSM-002 | `SatStateMachine` shall transition to Safe mode upon receipt of a `SAFE_MODE` command. | Inspection |
| FCD-SSM-003 | `SatStateMachine` shall transition to Safe mode when `vbatt` in the `PowerState` struct falls below `CRITICAL_THRESHOLD`. | Inspection |
| FCD-SSM-004 | `SatStateMachine` shall transition to Standby mode upon receipt of a `SAFE_EXIT` command. | Inspection |
| FCD-SSM-005 | `SatStateMachine` shall send the appropriate `Comms.Mode` command to `CommsApplication` on every mode change. | Inspection |
| FCD-SSM-006 | `SatStateMachine` shall emit an event on every mode transition. | Inspection |
| FCD-SSM-007 | `SatStateMachine` shall respond to health pings within the required deadline. | Inspection |

---

## 3. Design

### 3.1 Component Type

Active component with a simple flat F' state machine (Safe / Standby). No hierarchical substates are needed — `SatStateMachine` does not have operational work to perform within a mode beyond evaluating transition conditions each tick.

### 3.2 Mode Interface

`SatStateMachine` sends one typed mode port per application component that accepts mode commands. Application components receive only their own mode enum; they never see the satellite's internal `Safe`/`Standby` representation.

```fpp
module FeatherCdh {
    port CommsModePort(mode: Comms.Mode)
}
```

### 3.3 Translation Table

| Satellite Mode | `CommsApplication` receives |
|---------------|----------------------------|
| Safe | `Comms.Mode.Safe` |
| Standby | `Comms.Mode.Normal` |

### 3.4 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 1 Hz rate group tick; triggers condition evaluation |
| `commsModeOut` | Output | `FeatherCdh.CommsModePort` | Mode command to `CommsApplication` |
| `powerStateGet` | Output | Sync get → `EPSApplication` | Retrieves cached `PowerState` struct (vbatt, fault flags, charging status). Executes on `SatStateMachine`'s own thread via `EPSApplication`'s guarded handler. |
| `cmdIn` | Input | `Fw.Cmd` | Ground commands via `CmdDispatcher` |
| `cmdResponseOut` | Output | `Fw.CmdResponse` | Command completion status |
| `pingIn` / `pingOut` | In/Out | `Svc.Ping` | Health monitoring |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Telemetry (current mode) |
| `prmGet` | Output | `Fw.PrmGet` | Load `CRITICAL_THRESHOLD` from `PrmDb` |

### 3.5 Commands

| Mnemonic | Args | Description |
|----------|------|-------------|
| `SAFE_MODE` | — | Force transition to Safe mode from any mode. |
| `SAFE_EXIT` | — | Transition from Safe to Standby. Accepted only in Safe mode; rejected with `EXECUTION_ERROR` otherwise. |

---

## 4. State Machine

Flat two-state F' state machine (`Fw::Sm`):

```
SAFE (initial)
  └─ (on schedIn) invoke powerStateGet
                  if vbatt < CRITICAL_THRESHOLD → stay SAFE, emit WARNING_HI
                  emit telemetry
     (on SAFE_EXIT command) → STANDBY
     (on SAFE_MODE command) → stay SAFE

STANDBY
  └─ (on schedIn) invoke powerStateGet
                  if vbatt < CRITICAL_THRESHOLD → emit WARNING_HI; → SAFE
                  emit telemetry
     (on SAFE_MODE command) → SAFE
     (on SAFE_EXIT command) → reject (EXECUTION_ERROR)
```

Mode transition actions (on every Safe ↔ Standby transition):
1. Call `commsModeOut` with the appropriate `Comms.Mode` value
2. Emit a mode-transition event

---

## 5. Notes

- `powerStateGet` calls `EPSApplication`'s guarded sync input port, executing on `SatStateMachine`'s thread. The handler returns the last value cached by `EPSApplication`'s 1 Hz tick. No recomputation occurs inside the get handler.
- `CRITICAL_THRESHOLD` is a persisted F' parameter loaded from `PrmDb`. Its value is TBD pending battery characterization.
- If additional application components are added in the future (e.g., a thermal or ADCS application), one new typed mode output port is added per component and the translation table is extended. No other changes to the state machine are required.
- `SatStateMachine` never communicates directly with any Layer 2 hardware manager or Layer 1 driver.
