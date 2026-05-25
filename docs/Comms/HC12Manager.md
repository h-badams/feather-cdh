# HC12Manager SDD

## 1. Overview

`HC12Manager` is the Layer 2 Queued component that manages the HC-12 433 MHz UART radio module. It implements the `Drv::ByteStreamDriverModel` interface toward `ComCcsds`, making it the byte-stream driver that the `ComCcsds` `comStub` component talks to. Downward, it connects to `LinuxUartDriver` for physical UART I/O.

During initialization, `HC12Manager` drives the HC-12 SET pin LOW, sends AT configuration commands over the UART to verify/set operating parameters, then releases SET HIGH to return the module to transparent-bridge operation. In the RUN state it passes bytes bidirectionally between `ComCcsds` and the UART driver without modification.

---

## 2. HC-12 Hardware Facts

Key facts from the HC-12 V2.4 datasheet relevant to this driver:

| Property | Value |
|----------|-------|
| SET pin logic | **LOW = AT command mode**; HIGH or floating = transparent mode (internal 10 kΩ pull-up) |
| UART levels | 3.3 V TTL (internal 1 kΩ series resistors on RXD and TXD) |
| Supply voltage | 3.2 V – 5.5 V DC; ≥ 200 mA supply capacity required |
| Factory defaults | FU3 mode, 9600 bps (8N1), CH001 (433.4 MHz), +20 dBm |
| AT entry timing | Pull SET LOW → wait **40 ms** → send AT command |
| AT exit timing | Assert SET HIGH → wait **80 ms** → transparent mode ready |
| Re-entry guard | After exiting AT mode, wait **≥ 200 ms** before pulling SET LOW again |
| AT mode baud (power-cycle entry) | Fixed at 9600, N, 1 |
| RF duplex | Half-duplex at RF level; module handles arbitration internally |

**AT commands used during CONFIGURE:**

| Command | Response | Effect |
|---------|----------|--------|
| `AT` | `OK` | Verify AT mode is active |
| `AT+RX` | `OK+FU3\r\nOK+B9600\r\nOK+C001\r\nOK+RP:+20 dBm\r\n` | Read all current parameters |
| `AT+B9600` | `OK+B9600` | Set UART baud rate (adjust if a non-default rate is desired) |
| `AT+C001` | `OK+C001` | Set channel 001 = 433.4 MHz (adjust if needed) |
| `AT+FU3` | `OK+FU3` | FU3 = full-speed mode; auto air-baud matched to UART baud |
| `AT+P8` | `OK+P8` | Maximum transmit power (+20 dBm) |

If the module already holds factory defaults from a previous run, no register writes are strictly necessary. `AT+RX` can be used to verify and commands skipped if values already match.

---

## 3. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-HC12-001 | `HC12Manager` shall implement the `Drv::ByteStreamDriverModel` interface. | Inspection |
| FCD-HC12-002 | `HC12Manager` shall drive SET LOW, wait 40 ms, send AT configuration commands, drive SET HIGH, and wait 80 ms before entering RUN. | Inspection |
| FCD-HC12-003 | `HC12Manager` shall pass the `ComCcsds` byte stream to `LinuxUartDriver` transparently in RUN state. | Inspection |
| FCD-HC12-004 | `HC12Manager` shall emit a `WARNING_HI` event and transition to RESET on a UART transmission error. | Inspection |

---

## 4. Design

### 4.1 Component Type

Queued component with a flat F' state machine following the standard hardware manager pattern.

### 4.2 ByteStreamDriver Interface

`HC12Manager` sits between `ComCcsds` and `LinuxUartDriver` and implements two complementary F Prime interfaces:

- **Toward `ComCcsds`** (`comStub`): HC12Manager implements `Drv::ByteStreamDriver` — it is the driver that `comStub` (a `ByteStreamDriverClient`) talks to.
- **Toward `LinuxUartDriver`**: HC12Manager implements `Drv::ByteStreamDriverClient` — it is the client that calls into `LinuxUartDriver` (which implements `Drv::ByteStreamDriver`).

### 4.3 All Ports

**Upward interface — `Drv::ByteStreamDriver` toward `ComCcsds`:**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `$send` | guarded input | `Drv.ByteStreamSend` | Downlink: called by `comStub.drvSendOut` to transmit a framed packet. Returns `ByteStreamStatus`. Guarded (not async) because the port returns a value. |
| `recvReturnIn` | guarded input | `Fw.BufferSend` | Buffer ownership returned by `comStub.drvReceiveReturnOut` after uplink data is consumed |
| `ready` | Output | `Drv.ByteStreamReady` | Signals `comStub.drvConnected` that the driver is ready (fired on entry to RUN state) |
| `$recv` | Output | `Drv.ByteStreamData` | Uplink: pushes bytes received from UART to `comStub.drvReceiveIn` for deframing |

**Downward interface — `Drv::ByteStreamDriverClient` toward `LinuxUartDriver`:**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `drvSendOut` | Output | `Drv.ByteStreamSend` | Send bytes to `LinuxUartDriver.$send` |
| `drvReceiveReturnOut` | Output | `Fw.BufferSend` | Return receive buffer to `LinuxUartDriver.recvReturnIn` |
| `drvConnected` | sync input | `Drv.ByteStreamReady` | Receives ready signal from `LinuxUartDriver.ready` |
| `drvReceiveIn` | sync input | `Drv.ByteStreamData` | Receives bytes from `LinuxUartDriver.$recv` |

**Other ports:**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 1 Hz rate group tick; drives state machine |
| `setPin` | Output | `Drv.GpioWrite` | Controls HC-12 SET pin (LOW = AT mode, HIGH = transparent mode). Connects to a dedicated `LinuxGpioDriver` instance. |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Telemetry (bytes sent, bytes received, UART error count) |

### 4.4 Commands

None.

---

## 5. State Machine

Standard hardware manager flat F' state machine:

```
RESET (initial)
  └─ (on schedIn) assert setPin LOW (enter AT command mode)
                  Os::Task::delay(40 ms)
                  → WAIT_RESET

WAIT_RESET
  └─ (on schedIn) send "AT\r\n" via drvSendOut → LinuxUartDriver; wait for "OK" response via drvReceiveIn
                  if no response within timeout → WARNING_HI, retry up to N times → RESET
                  → CONFIGURE

CONFIGURE
  └─ (on schedIn) send AT+RX to read current parameters
                  send AT+B9600 (or desired baud rate)
                  send AT+C001 (or desired channel)
                  send AT+FU3
                  send AT+P8
                  verify each response (expected format: "OK+Bxxxx", "OK+Cxxx", etc.)
                  if any command fails → WARNING_HI, → RESET
                  assert setPin HIGH (exit AT command mode)
                  Os::Task::delay(80 ms)
                  → RUN
                  call ready port (signal comStub.drvConnected that driver is ready)
                  emit "HC12Manager transitioned to RUN" event

RUN
  └─ (on $send)       write buffer to drvSendOut → LinuxUartDriver (transmit to HC-12)
                      return ByteStreamStatus to caller
     (on drvReceiveIn) forward received bytes to $recv → comStub (deliver for deframing)
                       call drvReceiveReturnOut to return buffer to LinuxUartDriver
```

**UART error handling:** Any `drvSendOut` call returning a non-OK `ByteStreamStatus` logs `WARNING_HI` (throttled) and transitions to RESET.

---

## 6. Notes

- The HC-12 SET pin has an **internal 10 kΩ pull-up**. When `HC12Manager` is not actively driving the pin, it floats high, keeping the module in transparent mode. The `setPin` GPIO only needs to assert LOW during the CONFIGURE state.
- HC-12 AT commands are **persistent across power cycles** — settings survive power-off. On a fresh deploy, if the module already has correct factory defaults (FU3, 9600 bps, CH001, +20 dBm), the CONFIGURE state AT writes are effectively no-ops that just verify the state. No harm in sending them every time.
- The AT mode entry and exit timing (40 ms and 80 ms respectively) is satisfied naturally by the 1 Hz tick interval (1000 ms). `doReset` asserts SET LOW and immediately signals success; WAIT_RESET holds for one full tick before CONFIGURE begins, providing well over the required 40 ms settling time. Similarly, `doConfigure` asserts SET HIGH at the end and signals success; the full tick that elapses before the first RUN action provides well over the required 80 ms before transparent mode traffic begins. No explicit delays are needed.
- `LinuxUartDriver` receive behavior (interrupt-driven vs polled) affects how `drvReceiveIn` delivers bytes — push callback or polling. Exact wiring TBD pending driver implementation.
- The HC-12 operates half-duplex at the RF level. The UART wires to the Feather M4 are full-duplex — the module's internal MCU handles RF arbitration. No software-level half-duplex management is required in `HC12Manager`.
- The ground-side HC-12 is connected via USB-TTL adapter to the PC running the F Prime GDS. The GDS communicates using standard F Prime CCSDS framing — no ground-side software changes are required beyond pointing the GDS at the correct serial port.
- If operating two HC-12 units within 10 meters of each other at 9600 bps, the datasheet recommends staggering by at least 5 channels from any other nearby HC-12 units to avoid interference.
