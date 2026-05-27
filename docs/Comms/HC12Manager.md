# HC12Manager SDD

## 1. Overview

`HC12Manager` is the Layer 2 Queued component that manages the HC-12 433 MHz UART radio module. It plays the **Com Adapter** role for the comms stack: it implements the `Svc.Com` interface upward toward the `ComCcsds.FramingSubtopology` (taking the place of `Svc.ComStub`) and implements the `Drv.ByteStreamDriverClient` interface downward toward `LinuxUartDriver`.

The HC-12 is a byte-transparent radio — UART bytes in one side appear at the UART of the other side. There is no protocol layer between the flight computer and the air interface, so HC-12 needs no dedicated driver beyond the standard `LinuxUartDriver`. `HC12Manager` exists to (a) drive the SET pin and send AT configuration commands at startup, and (b) act as the Com Adapter glue between the CCSDS framing pipeline and the byte-stream UART driver.

During initialization, `HC12Manager` drives the HC-12 SET pin LOW, sends AT configuration commands over the UART, then releases SET HIGH to return the module to transparent-bridge operation. In the RUN state it passes framed packets bidirectionally between the framing pipeline and the UART driver without modification.

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

If the module already holds factory defaults from a previous run, no register writes are strictly necessary. `AT+RX` can be used to verify and commands skipped if values already match. In the current design, AT command sequencing in `doConfigure` does not parse responses — each `drvSendOut` return status is treated as the success/failure indicator.

---

## 3. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-HC12-001 | `HC12Manager` shall implement the `Svc.Com` (Com Adapter) interface upward and the `Drv.ByteStreamDriverClient` interface downward. | Inspection |
| FCD-HC12-002 | `HC12Manager` shall drive SET LOW, wait 40 ms, send AT configuration commands, drive SET HIGH, and wait 80 ms before entering RUN. | Inspection |
| FCD-HC12-003 | `HC12Manager` shall transparently forward downlink data from `dataIn` to `drvSendOut` and uplink data from `drvReceiveIn` to `dataOut` while in RUN state. | Inspection |
| FCD-HC12-004 | `HC12Manager` shall emit a `WARNING_HI` event and transition to RESET on any UART transmission error. | Inspection |
| FCD-HC12-005 | `HC12Manager` shall emit `Fw::Success::SUCCESS` on `comStatusOut` on entry to RUN, signaling the framer that the link is ready to accept downlink data. | Inspection |

---

## 4. Design

### 4.1 Component Type

Queued component with a flat F' state machine following the standard hardware manager pattern.

### 4.2 Com Adapter Interface

`HC12Manager` fills the role normally occupied by `Svc.ComStub` in the full `ComCcsds.Subtopology`. The project uses `ComCcsds.FramingSubtopology` (which omits ComStub) and the user is responsible for wiring five connections to a component implementing `Svc.Com`. `HC12Manager` provides those endpoints:

- **`dataIn`** (sync input, `Svc.ComDataWithContext`): Downlink. `framer.dataOut` delivers a framed packet here. In RUN state, the buffer's bytes are forwarded to `drvSendOut`. The send result drives `comStatusOut`; the original buffer is returned via `dataReturnOut`.
- **`dataReturnIn`** (sync input, `Svc.ComDataWithContext`): Uplink buffer return. `frameAccumulator.dataReturnOut` returns ownership of a buffer that was previously emitted on `dataOut`. The buffer is then released back to the UART driver via `drvReceiveReturnOut`.
- **`dataOut`** (output, `Svc.ComDataWithContext`): Uplink. Bytes received from `drvReceiveIn` are forwarded here to `frameAccumulator.dataIn` for CCSDS deframing.
- **`dataReturnOut`** (output, `Svc.ComDataWithContext`): Downlink buffer return. Returns ownership of the downlink buffer to `framer.dataReturnIn` after transmission.
- **`comStatusOut`** (output, `Fw.SuccessCondition`): Reports `SUCCESS` after a successful downlink send (and once on RUN entry to signal the framer the link is ready), or `FAILURE` after a UART transmission error.

Downward, `HC12Manager` implements `Drv.ByteStreamDriverClient` to talk to `LinuxUartDriver`:

- **`drvSendOut`** (output, `Drv.ByteStreamSend`): Send bytes to `LinuxUartDriver.$send`. Returns `Drv.ByteStreamStatus`.
- **`drvReceiveIn`** (sync input, `Drv.ByteStreamData`): Bytes received from `LinuxUartDriver.$recv`.
- **`drvReceiveReturnOut`** (output, `Fw.BufferSend`): Returns receive-buffer ownership to `LinuxUartDriver.recvReturnIn`.
- **`drvConnected`** (sync input, `Drv.ByteStreamReady`): Ready signal from `LinuxUartDriver.ready`. Treated as informational — `HC12Manager` fires its own `comStatusOut(SUCCESS)` on RUN entry, independent of when this arrives.

### 4.3 All Ports

**Upward — `Svc.Com` (toward `ComCcsds.FramingSubtopology`):**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `dataIn` | sync input | `Svc.ComDataWithContext` | Downlink: framed packet from `framer.dataOut` to be transmitted |
| `dataReturnIn` | sync input | `Svc.ComDataWithContext` | Uplink buffer ownership return from `frameAccumulator.dataReturnOut` |
| `dataOut` | output | `Svc.ComDataWithContext` | Uplink: bytes received from UART, forwarded to `frameAccumulator.dataIn` |
| `dataReturnOut` | output | `Svc.ComDataWithContext` | Downlink buffer ownership return to `framer.dataReturnIn` |
| `comStatusOut` | output | `Fw.SuccessCondition` | Tx status / ready signal to `framer.comStatusIn` |

**Downward — `Drv.ByteStreamDriverClient` (toward `LinuxUartDriver`):**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `drvSendOut` | output | `Drv.ByteStreamSend` | Send bytes to `LinuxUartDriver.$send` |
| `drvReceiveIn` | sync input | `Drv.ByteStreamData` | Bytes received from `LinuxUartDriver.$recv` |
| `drvReceiveReturnOut` | output | `Fw.BufferSend` | Return receive buffer to `LinuxUartDriver.recvReturnIn` |
| `drvConnected` | sync input | `Drv.ByteStreamReady` | Informational ready signal from `LinuxUartDriver.ready` |

**Other ports:**

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | sync input | `Svc.Sched` | 1 Hz rate group tick; drives state machine |
| `setPin` | output | `Drv.GpioWrite` | Controls HC-12 SET pin (LOW = AT mode, HIGH = transparent mode). Connects to a dedicated `LinuxGpioDriver` instance. |
| `logOut` | output | `Fw.Log` | Event logging |
| `tlmOut` | output | `Fw.Tlm` | Telemetry (bytes sent, bytes received, UART error count) |
| `timeCaller` | output | `Fw.Time` | Time tag for events |

### 4.4 Commands

None.

---

## 5. State Machine

Standard hardware manager flat F' state machine:

```
RESET (initial)
  └─ (on schedIn) assert setPin LOW (enter AT command mode)
                  → success → WAIT_RESET

WAIT_RESET
  └─ (on schedIn) one-tick settling delay (>> 40 ms required)
                  → success → CONFIGURE
                  (on error  → RESET)

CONFIGURE
  └─ (on schedIn) send each AT command via drvSendOut in sequence:
                    AT, AT+B9600, AT+C001, AT+FU3, AT+P8
                  if any drvSendOut returns non-OK ByteStreamStatus
                    → emit atCommandFailed (WARNING_HI, throttled)
                    → error → RESET
                  assert setPin HIGH (exit AT command mode)
                  emit runEntry event
                  emit comStatusOut(Fw::Success::SUCCESS)  ← signals framer to start downlink
                  → success → RUN

RUN
  └─ (on schedIn)       emit telemetry (BYTES_SENT, BYTES_RECV, UART_ERRORS)
     (on dataIn)        forward buffer bytes to drvSendOut
                        emit comStatusOut(SUCCESS) on OK return, else FAILURE
                        return buffer ownership via dataReturnOut
                        on UART error → emit uartSendError, error → RESET
     (on drvReceiveIn)  forward buffer to dataOut (toward frameAccumulator)
                        increment BYTES_RECV counter
     (on dataReturnIn)  forward buffer ownership to drvReceiveReturnOut
     (on drvConnected)  no-op (informational; ready was already signaled on RUN entry)
```

**UART error handling:** Any `drvSendOut` call returning a non-OK `ByteStreamStatus` logs `uartSendError` (WARNING_HI, throttled), increments `UART_ERRORS` telemetry, and transitions the state machine to RESET via the `error` signal.

The Com Adapter ports (`dataIn`, `dataReturnIn`, `drvReceiveIn`) **always forward** regardless of state — they are not gated on RUN. This is safe because the framer will not invoke `dataIn` until it has received `comStatusOut(SUCCESS)`, which only happens on RUN entry.

---

## 6. Notes

- The HC-12 SET pin has an **internal 10 kΩ pull-up**. When `HC12Manager` is not actively driving the pin, it floats high, keeping the module in transparent mode. The `setPin` GPIO only needs to assert LOW during AT command mode.
- HC-12 AT commands are **persistent across power cycles** — settings survive power-off. On a fresh deploy, if the module already has correct factory defaults (FU3, 9600 bps, CH001, +20 dBm), the CONFIGURE state AT writes are effectively no-ops that just push known bytes through the UART. No harm in sending them every time. `doConfigure` does not parse `OK` responses; it relies on `drvSendOut`'s `ByteStreamStatus` return as the success/failure signal.
- The AT mode entry (40 ms) and exit (80 ms) timing is satisfied naturally by the 1 Hz tick interval (1000 ms). `doReset` asserts SET LOW and immediately signals success; `WAIT_RESET` holds for one full tick before `CONFIGURE` begins, providing well over the required 40 ms settling time. Similarly, after `doConfigure` asserts SET HIGH, the next-tick boundary to RUN provides well over the required 80 ms before transparent mode traffic begins. No explicit delays are needed.
- `LinuxUartDriver` receive behavior (interrupt-driven vs polled) affects how `drvReceiveIn` delivers bytes — push callback or polling. Exact wiring TBD pending driver implementation.
- The HC-12 operates half-duplex at the RF level. The UART wires to the Feather M4 are full-duplex — the module's internal MCU handles RF arbitration. No software-level half-duplex management is required in `HC12Manager`.
- The ground-side HC-12 is connected via USB-TTL adapter to the PC running the F Prime GDS. The GDS communicates using standard F Prime CCSDS framing — no ground-side software changes are required beyond pointing the GDS at the correct serial port.
- If operating two HC-12 units within 10 meters of each other at 9600 bps, the datasheet recommends staggering by at least 5 channels from any other nearby HC-12 units to avoid interference.
- Because `HC12Manager` replaces `Svc.ComStub`, the project must import `ComCcsds.FramingSubtopology` rather than `ComCcsds.Subtopology`. The FramingSubtopology omits the ComStub instance and requires the user to wire five connections to a component implementing `Svc.Com` (see [docs/sdd.md](../sdd.md) §10).
