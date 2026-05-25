# CommsApplication SDD

## 1. Overview

`CommsApplication` is the Layer 3 Active component for the Comms subtopology. It receives mode commands from `SatStateMachine` and controls autonomous downlink behavior accordingly. In `Normal` mode it drives the `ComCcsds` ComQueue drain each rate group tick, actively sending queued telemetry and events to the ground. In `Safe` mode it gates the ComQueue drain, suppressing autonomous downlink.

`CommsApplication` is the single point of control for when the satellite downlinks data. Future higher-level comms CONOPS logic (e.g., link scheduling, power-aware downlink inhibit) would be added here.

---

## 2. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-COM-001 | `CommsApplication` shall fire `comQueueRun` on each rate group tick when in `Normal` mode. | Inspection |
| FCD-COM-002 | `CommsApplication` shall not fire `comQueueRun` when in `Safe` mode. | Inspection |
| FCD-COM-003 | `CommsApplication` shall transition between `Safe` and `Normal` modes upon receipt of a mode command from `SatStateMachine`. | Inspection |
| FCD-COM-004 | `CommsApplication` shall emit an event on each mode transition. | Inspection |
| FCD-COM-005 | `CommsApplication` shall respond to health pings within the required deadline. | Inspection |

---

## 3. Design

### 3.1 Component Type

Active component with a hierarchical F' state machine (`Fw::Sm`). Mode is the top-level state. There are no operational substates at this time — the `Safe` and `Normal` top-level states contain only `initial` pseudo-substates. Substates may be added as comms CONOPS logic is refined.

### 3.2 Mode Interface

`CommsApplication` receives `Comms.Mode` from `SatStateMachine` via the `commsModeIn` port. It does not know whether the satellite is in `Safe` or `Standby` — it only knows its own mode.

```fpp
module Comms {
    enum Mode {
        Safe,
        Normal
    }
}
```

### 3.3 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 10 Hz rate group tick |
| `commsModeIn` | async input | `FeatherCdh.CommsModePort` | Mode command from `SatStateMachine` |
| `comQueueRun` | Output | `Svc.Sched` | Fires `ComCcsds` ComQueue drain. Connected to `comQueue.run` in the `ComCcsds` subtopology. Fired only in `Normal` mode. |
| `pingIn` / `pingOut` | In/Out | `Svc.Ping` | Health monitoring |
| `cmdIn` | Input | `Fw.Cmd` | Ground commands via `CmdDispatcher` |
| `cmdResponseOut` | Output | `Fw.CmdResponse` | Command completion status |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Telemetry (current comms mode) |

### 3.4 Commands

None currently. Future commands (e.g., `FORCE_DOWNLINK`, `INHIBIT_DOWNLINK`) may be added as comms CONOPS is refined.

---

## 4. State Machine

Hierarchical F' state machine with two top-level modes. A single `switchMode` signal at the top level is inherited by all leaf states.

```
SAFE (initial)
  └─ IDLE (initial substate)
       (on schedIn) do nothing — ComQueue drain suppressed

NORMAL
  └─ ACTIVE (initial substate)
       (on schedIn) call comQueueRun port — drains ComQueue into framing pipeline

Top-level signal: switchMode(mode: Comms.Mode)
  → transitions to SAFE or NORMAL accordingly
  → on entry to each top-level state: emit mode-transition event
```

---

## 5. Notes

- `ComCcsds` ComQueue is an Active component with its own thread. Its `run` input port is NOT connected to any rate group in the topology. Instead, `CommsApplication` is the sole driver of `comQueue.run`. This means ComQueue only drains when `CommsApplication` fires the port.
- The F Prime uplink path (ground → radio → flight computer → `CmdDispatcher`) is always active regardless of comms mode, since it is driven by `HC12Manager` receiving bytes from the UART and pushing them upstream through the `ComCcsds` framing pipeline. `CommsApplication` does not gate uplink.
- The downlink drain rate is 10 Hz (once per `schedIn` tick in `Normal` mode). This matches the F Prime reference pattern of connecting ComQueue to the fastest rate group, ensuring queued events and telemetry are flushed promptly rather than building up. If throughput needs to be reduced, `CommsApplication` can be moved to a slower rate group.
- Future comms CONOPS logic that could be added to `CommsApplication`: power-aware downlink inhibit (query `EPSApplication` before firing), scheduled downlink windows, link quality monitoring from `HC12Manager`.
- `CommsApplication` does not know the state of `HC12Manager` — if the radio is not initialized, data written to the `ComCcsds` pipeline will back up in ComQueue but no bytes will be transmitted. `HC12Manager` handles its own recovery independently.
