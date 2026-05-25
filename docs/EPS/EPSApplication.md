# EPSApplication SDD

## 1. Overview

`EPSApplication` is the Layer 3 Active component for the EPS subtopology. It accepts `SET_IC_REGISTER` commands from the ground and forwards them to `MpptIcManager` via a register-write port. It monitors battery and power system health by consuming state data published by `MpptIcManager` on each rate group tick, and exposes a `powerStateGet` synchronous get port read by `SatStateMachine` so mode transition decisions can be made.

Unlike other Layer 3 components, `EPSApplication` has no mode port and no internal state machine — it operates identically regardless of satellite mode.

---

## 2. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-EPS-001 | `EPSApplication` shall expose a synchronous get port that returns the latest cached `PowerState` struct when invoked by `SatStateMachine`. | Inspection |
| FCD-EPS-002 | `EPSApplication` shall emit a `WARNING_HI` event when `vbatt` falls below `POWER_THRESHOLD`. | Inspection |
| FCD-EPS-003 | `EPSApplication` shall emit a `WARNING_HI` event when `vbatt` falls below `CRITICAL_THRESHOLD`. | Inspection |
| FCD-EPS-004 | `EPSApplication` shall forward `SET_IC_REGISTER` commands to `MpptIcManager` without refusal logic. | Inspection |
| FCD-EPS-005 | `EPSApplication` shall operate continuously regardless of satellite mode. | Inspection |
| FCD-EPS-006 | `EPSApplication` shall respond to health pings within the required deadline. | Inspection |

---

## 3. Design

### 3.1 Component Type

Active component. No hierarchical state machine — `EPSApplication` is fully driven by rate group ticks and incoming commands with no satellite-mode-driven state transitions.

### 3.2 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 1 Hz rate group tick |
| `batteryStateIn` | async input | Custom struct port | BQ25756E measurements and status from `MpptIcManager`. Fields: `vbatt_mV: U32`, `ibatt_raw: U16`, `vac_mV: U32`, `iac_raw: U16`, `vfb_mV: U32`, `ts_raw: U16`, `chargingState: BQ25756EChargingState`, `chargerStatus2: U8`, `chargerStatus3: U8`, `faultStatus: U8`, `faultFlag: U8`, `mppt_enabled: bool`. |
| `powerStateGet` | guarded input | Custom sync get port returning `PowerState` struct | Invoked by `SatStateMachine` on its own thread; returns cached `PowerState` without recomputation. Thread safety provided by guarded (mutex-protected) handler. |
| `setRegister` | Output | Custom port (`BQ25756Reg`, `U32`) | Register write forwarded to `MpptIcManager`. `reg` is a named enum value (e.g., `BQ25756Reg::CHG_ENABLE`, `BQ25756Reg::ICHG_LIM`). |
| `cmdIn` | Input | `Fw.Cmd` | Ground commands via `CmdDispatcher` |
| `cmdResponseOut` | Output | `Fw.CmdResponse` | Command completion status |
| `pingIn` / `pingOut` | In/Out | `Svc.Ping` | Health monitoring |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Telemetry (vbatt, ibatt, charging status, fault flags) |
| `prmGet` | Output | `Fw.PrmGet` | Load `POWER_THRESHOLD` and `CRITICAL_THRESHOLD` from `PrmDb` |

### 3.3 Commands

| Mnemonic | Args | Description |
|----------|------|-------------|
| `SET_IC_REGISTER` | `reg: BQ25756Reg`, `value: U32` | Write a BQ25756E register via `MpptIcManager`. `reg` is a named enum (e.g., `BQ25756Reg::CHARGE_VOLT_LIM`, `BQ25756Reg::MPPT_CONT`). No refusal logic — all writes are forwarded unconditionally. |

---

## 4. Operational Behavior

`EPSApplication` does not use a state machine. All behavior is driven by incoming port calls.

**Rate group tick flow (1 Hz):**

```
schedIn fires
  → read latest batteryState (already received via batteryStateIn)
  → check vbatt against POWER_THRESHOLD
      if below → emit WARNING_HI (LOW_BATTERY)
  → check vbatt against CRITICAL_THRESHOLD
      if below → emit WARNING_HI (CRITICAL_BATTERY)
  → assemble and cache PowerState struct
  → emit telemetry channels (vbatt, ibatt, charging status, fault flags)
```

**Synchronous get flow (`powerStateGet`):**

```
SatStateMachine invokes powerStateGet (caller's thread, guarded)
  → acquire component mutex
  → return cached PowerState struct
  → release mutex
```

**Command flow (`SET_IC_REGISTER`):**

```
cmdIn SET_IC_REGISTER(reg, value)
  → call setRegister(reg, value) to MpptIcManager
  → emit activity event (register written)
  → send cmdResponse (OK or EXECUTION_ERROR on I2C failure)
```

---

## 5. Notes

- `EPSApplication` does not autonomously enable or disable MPPT or charging. All such changes require an explicit `SET_IC_REGISTER` command from the ground.
- `batteryStateIn` is an async input port — `MpptIcManager` pushes the struct to `EPSApplication`'s queue on each tick. `EPSApplication` processes it during its own rate group handler. The `powerStateGet` handler returns the value last assembled during `schedIn`, not the raw `batteryStateIn` delivery.
- `powerStateGet` uses a `guarded_input` port (mutex-protected) rather than a plain `sync_input`, since the cached struct is written by `EPSApplication`'s own thread and read from `SatStateMachine`'s thread.
- `BQ25756Reg` enum definition (register names and addresses) is documented in the project `CLAUDE.md` and will be formalized as an FPP enum during detailed design.
- `POWER_THRESHOLD` and `CRITICAL_THRESHOLD` are persisted F' parameters. Specific values TBD pending battery characterization.
- `INA3221Manager` emits its rail telemetry independently and does not flow through `EPSApplication`. If PDS rail data ever needs to be part of the `PowerState` struct, this will require an additional `batteryStateIn`-style port from `INA3221Manager` to `EPSApplication`.
