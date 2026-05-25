# MpptIcManager SDD

## 1. Overview

`MpptIcManager` is the Layer 2 Active component responsible for all communication with the BQ25756E MPPT and battery charging IC on the BMS board. No other component reads from or writes to the BQ25756E directly.

On each rate group tick in `RUN` state, `MpptIcManager` reads all relevant measurement and status registers over I2C, assembles the data into a battery state struct, and pushes it to `EPSApplication` via `batteryStateOut`.

When `EPSApplication` forwards a register-write command via the `setRegister` port, `MpptIcManager` writes the specified BQ25756E register over I2C and emits an event. When the BQ25756E INT interrupt fires, `MpptIcManager` reads the fault registers, emits a fault event, and re-initializes the IC by transitioning back to `RESET`.

---

## 2. BQ25756E Hardware Facts

Key facts confirmed from the HS2-EPS-flightSW reference code and register file:

| Property | Value |
|----------|-------|
| I2C address | `0x6B` (7-bit) |
| I2C byte order | Big-endian (MSB first) for 16-bit registers |
| Hardware RESET pin | **None** — BQ25756E has no hardware reset GPIO. Software register reset: `REG_RST` (bit 7 of `POW_PATH_REV_CONT` 0x19). Writing 1 resets all registers to power-on defaults; bit self-clears after reset. |
| ADC enable | `ADC_CONT` (0x2B) — write to enable ADC and select one-shot or continuous mode |
| ADC channel enable | `ADC_CHANNEL_CONT` (0x2C) — write to enable individual measurement channels |
| Watchdog timer | `TIME_CONT` (0x15) — must be configured or disabled during initialization; default timeout will reset charger settings if not kicked |

**ADC register scaling (confirmed from reference code):**

| Register | Address | Scaling |
|----------|---------|---------|
| `VBAT_ADC` | 0x33 | 2 mV/LSB — raw value × 2 = millivolts |
| `VAC_ADC` | 0x31 | 2 mV/LSB — raw value × 2 = millivolts |
| `IAC_ADC` | 0x2D | Scaling TBD (confirm against datasheet) |
| `IBAT_ADC` | 0x2F | Scaling TBD (confirm against datasheet) |
| `TS_ADC` | 0x37 | Scaling TBD (confirm against datasheet) |
| `VFB_ADC` | 0x39 | 2 mV/LSB (same as VBAT/VAC based on reference code) |

**CHARGE_CURR_LIM (0x02) encoding:** bits [10:2] hold the charge current value; each bit step = 50 mA.

**CHARGER_STATUS_1 (0x21) bits [2:0] — charging state:**

| Value | State |
|-------|-------|
| 0x00 | Not Charging |
| 0x01 | Trickle Charge (VFB < VBAT_SHORT) |
| 0x02 | Pre-charge (VBAT_SHORT ≤ VFB < VBAT_LOWV) |
| 0x03 | Fast-charge (CC mode) |
| 0x04 | Taper Charge (CV mode) |
| 0x05 | Reserved |
| 0x06 | Top-off Timer Active Charging |
| 0x07 | Charge Termination Done |

**CHARGER_CONT (0x17) bits [7:6] — Vrechg threshold** (as % of Vfb_reg):
- 0b00 → 93.0%, 0b01 → 94.3%, 0b10 → 95.2%, 0b11 → 97.6%

**PRECHARGE_TERM_CONT (0x14) bits [2:1] — VBAT_LOWV threshold** (as % of Vfb_reg):
- 0b00 → 30%, 0b01 → 55%, 0b10 → 66.7%, 0b11 → 71.4%

---

## 3. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-MIM-001 | `MpptIcManager` shall be the sole flight-software owner of the BQ25756E IC. | Inspection |
| FCD-MIM-002 | `MpptIcManager` shall publish the BQ25756E measurement, charging status, and fault state on `batteryStateOut` on each rate group tick. | Inspection |
| FCD-MIM-003 | `MpptIcManager` shall write the specified register over I2C on each `setRegister` call. | Inspection |
| FCD-MIM-004 | `MpptIcManager` shall, on assertion of the BQ25756E INT pin, emit a `WARNING_HI` fault event and re-initialize the IC. | Inspection |
| FCD-MIM-005 | `MpptIcManager` shall emit telemetry channels for vbatt, ibatt, vac, iac, and charging status. | Inspection |

---

## 4. Design

### 4.1 Component Type

Active component with a flat F' state machine following the standard hardware manager pattern (RESET → WAIT_RESET → ENABLE → CONFIGURE → RUN). The BQ25756E software register reset (`REG_RST`) serves as the RESET action in place of a hardware GPIO reset line.

### 4.2 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 1 Hz rate group tick; drives state machine |
| `busWrite` | Output | `Drv.I2c` | Write BQ25756E registers; calls `LinuxI2cDriver.write` |
| `busWriteRead` | Output | `Drv.I2cWriteRead` | Read BQ25756E registers (write register address then read data in one repeated-start transaction); calls `LinuxI2cDriver.writeRead` |
| `intPin` | async input | `Svc.Cycle` | Fires when BQ25756E asserts INT due to a fault. Driven by `LinuxGpioDriver.gpioInterrupt` (output port type `Svc.Cycle`). |
| `batteryStateOut` | Output | Custom struct port | Battery state to `EPSApplication`. Fields: `vbatt_mV: U32`, `ibatt_raw: U16`, `vac_mV: U32`, `iac_raw: U16`, `vfb_mV: U32`, `ts_raw: U16`, `chargingState: BQ25756EChargingState`, `chargerStatus2: U8`, `chargerStatus3: U8`, `faultStatus: U8`, `faultFlag: U8`, `mppt_enabled: bool`. |
| `setRegister` | async input | Custom port (`BQ25756Reg`, `U32`) | Register write called by `EPSApplication`; writes specified register over I2C |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Telemetry channels (vbatt, ibatt, vac, iac, charging status) |

> **Note:** There is no `icReset` GPIO port. The BQ25756E has no hardware RESET pin. The RESET state uses `REG_RST` (bit 7 of `POW_PATH_REV_CONT` 0x19) as the software equivalent — writing 1 resets all registers to power-on defaults and the bit self-clears.

---

## 5. State Machine

```
RESET (initial)
  └─ (on tick) write REG_RST=1 (POW_PATH_REV_CONT 0x19 bit 7) — resets all registers to
               power-on defaults; bit self-clears
               → WAIT_RESET

WAIT_RESET
  └─ (on tick) wait one tick for registers to settle
               → ENABLE

ENABLE
  └─ (on tick) read PART_INFO (0x3D) to verify device ID
               emit WARNING_HI if device ID mismatch
               → CONFIGURE

CONFIGURE
  └─ (on tick) write configuration registers in sequence:
                 TIME_CONT (0x15)        — disable internal watchdog
                 ADC_CONT (0x2B)         — enable ADC, continuous mode
                 ADC_CHANNEL_CONT (0x2C) — enable all channels (VBAT, IBAT, VAC, IAC, TS, VFB)
                 CHARGE_VOLT_LIM (0x00)  — target charge voltage
                 CHARGE_CURR_LIM (0x02)  — max charge current
                 TERM_CURR_LIM (0x12)    — termination threshold
               read back key config registers; emit WARNING_HI if readback mismatch
               → RUN

RUN
  └─ (on tick) read measurement registers (big-endian, 16-bit):
                 VBAT_ADC (0x33) → vbatt = raw × 2 mV
                 IBAT_ADC (0x2F) → ibatt (scaling TBD)
                 VAC_ADC  (0x31) → vac   = raw × 2 mV
                 IAC_ADC  (0x2D) → iac   (scaling TBD)
                 TS_ADC   (0x37) → temperature (scaling TBD)
                 VFB_ADC  (0x39) → vfb   = raw × 2 mV
               read status registers (8-bit):
                 CHARGER_STATUS_1 (0x21) → charging state (bits [2:0], see table §2)
                 CHARGER_STATUS_2 (0x22)
                 CHARGER_STATUS_3 (0x23)
                 FAULT_STATUS     (0x24)
                 FAULT_FLAG       (0x27)
                 MPPT_CONT        (0x1A) → mppt_enabled = bit 0
               assemble BatteryState struct
               call batteryStateOut port
               emit telemetry channels

     (on setRegister call) write specified register over I2C
                           emit "register written: <reg>=<value>" activity event

     (on intPin interrupt) read FAULT_STATUS (0x24) and FAULT_FLAG (0x27)
                           emit WARNING_HI fault event (include raw fault register values)
                           → RESET
```

**I2C error handling:** Any I2C transaction returning a non-OK status logs `WARNING_HI` (throttled) and transitions to `RESET` (self-healing).

**`setRegister` during non-RUN states:** Calls received while the IC is initializing (RESET through CONFIGURE) are dropped with a `WARNING_LO` event noting the write was discarded. Attempting the write mid-init could conflict with the initialization sequence, so dropping is safer than queueing. The operator must resend the command after the IC returns to `RUN`.

**`intPin` during non-RUN states:** Ignored. The IC is already being reset and reconfigured, so the fault condition will be cleared by the `REG_RST` write in the RESET state.

---

## 6. Notes

- **No hardware reset pin; software reset via REG_RST.** The BQ25756E has no hardware RESET GPIO. Software register reset is available: `REG_RST` is bit 7 of `POW_PATH_REV_CONT` (0x19). Writing 1 resets all registers to power-on defaults; the bit self-clears after reset. `CHARGER_CONT` bit 5 (`WD_RST`) is a separate, narrower operation that only resets the internal I2C watchdog timer — it does not reset IC state. In the `RESET` state, `MpptIcManager` writes `REG_RST=1` to clear any unknown IC state before the init sequence proceeds through WAIT_RESET → ENABLE → CONFIGURE → RUN.
- `BQ25756Reg` enum definition maps human-readable names to the register addresses documented in the project `CLAUDE.md`. This enum is shared with `EPSApplication` and will be defined as an FPP enum during detailed design.
- **Topology ordering:** `MpptIcManager` must be wired to its rate group slot before `EPSApplication` in the topology. This ensures `MpptIcManager.schedIn` fires first each 1 Hz cycle, pushing a fresh `BatteryState` into `EPSApplication`'s queue before `EPSApplication.schedIn` processes it.
- **Shared I2C bus:** `MpptIcManager` and `INA3221Manager` use separate `LinuxI2cDriver` instances (both on the same physical `/dev/i2c-N` bus). F Prime `guarded input` ports cannot be shared across components, so one driver instance per component is required. BQ25756E address is 0x6B; INA3221 address is TBD (0x40–0x43, see INA3221Manager SDD). No bus contention at 1 Hz polling rate.
- The BQ25756E internal watchdog resets charger settings to power-on defaults if the watchdog timer expires. This must be handled during initialization by either disabling the watchdog (`TIME_CONT` 0x15) or ensuring `MpptIcManager` re-writes configuration registers frequently enough. Given the 1 Hz tick rate, disabling the watchdog is the simpler approach.
- `intPin` port type is `Svc.Cycle`, driven by `LinuxGpioDriver.gpioInterrupt` (confirmed from `Drv/Interfaces/Gpio.fpp`).
- Configuration register values (charge voltage/current targets, JEITA thresholds) are TBD pending hardware bring-up and battery characterization. They will be exposed as F' parameters in `EPSApplication` via the `SET_IC_REGISTER` command.
- `batteryStateOut` type is a custom struct. If the struct becomes unwieldy, consider splitting into a measurements port (vbatt, ibatt, vac, iac, vfb, temperature) and a flags port (charging status, fault flags, MPPT state).
- IBAT and IAC ADC scaling factors need to be confirmed from the BQ25756E datasheet — the reference code does not decode these. VBAT and VAC are confirmed at 2 mV/LSB.
