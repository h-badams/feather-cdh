# WatchdogPinger SDD

## 1. Overview

`WatchdogPinger` is a Layer 2 Passive component that satisfies the TPS3431SDRBR hardware watchdog on the PDS board by toggling a dedicated GPIO pin on each rate group tick. It has no satellite mode awareness and no state machine.

**This component is PDS-dependent.** It is included in `FeatherCdhFullDeployment` only. It is absent from `FeatherCdhPartialDeployment`.

---

## 2. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-WDP-001 | `WatchdogPinger` shall pulse the TPS3431S WDI GPIO pin on each rate group tick. | Test |

---

## 3. Design

### 3.1 Component Type

Passive component. No queue, no thread, no state machine.

### 3.2 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 10 Hz rate group tick; triggers watchdog GPIO pulse |
| `watchdogPing` | Output | `Drv.GpioWrite` | GPIO pin connected to TPS3431S WDI input on PDS board |

### 3.3 Commands

None.

---

## 4. Operational Behavior

On each `schedIn` tick (10 Hz), `WatchdogPinger` asserts then immediately deasserts `watchdogPing` to produce a pulse that resets the TPS3431S watchdog timer. No OS delay is inserted between assert and deassert.

If this component stops being scheduled — due to a software hang or rate group slip — the hardware watchdog will expire and reset the system into Safe mode.

---

## 5. Notes

- The TPS3431SDRBR minimum WDI pulse width is 50 ns. A back-to-back GPIO assert/deassert via the driver is sufficient with no OS-level delay.
- The TPS3431SDRBR is a **window watchdog**: it has both a minimum inter-ping interval (open window) and a maximum inter-ping interval (close window). The PDS hardware configuration of these window parameters must be compatible with the 10 Hz ping rate used here, including any rate group slip margin. Confirm window timing with the hardware team before integration.
- `WatchdogPinger` is scheduled via the standard F' rate group mechanism and has no direct knowledge of whether PDS hardware is present. In `FeatherCdhPartialDeployment` the component is simply excluded from the topology rather than connected to a stub driver.
