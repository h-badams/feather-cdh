# INA3221Manager SDD

## 1. Overview

`INA3221Manager` is the Layer 2 Queued component responsible for all communication with the INA3221 26V Triple-Channel Current and Voltage Monitor on the PDS board. It reads the three PDS output rail voltages and currents on each rate group tick and emits them as telemetry. It does not feed data to `EPSApplication`.

**This component is PDS-dependent and is a stub.** It is included in `FeatherCdhFullDeployment` only. It is absent from `FeatherCdhPartialDeployment`. The component code compiles and follows the standard hardware manager state machine pattern; it simply has no real hardware to talk to until the PDS board is available.

---

## 2. Requirements

| ID | Requirement | Verification |
|----|-------------|--------------|
| FCD-INA-001 | `INA3221Manager` shall be the sole flight-software owner of the INA3221 IC. | Inspection |
| FCD-INA-002 | `INA3221Manager` shall emit telemetry channels for the voltage and current of all three PDS output rails on each rate group tick. | Inspection |

---

## 3. Design

### 3.1 Component Type

Queued component with a flat F' state machine following the standard hardware manager pattern. No dedicated thread — `schedIn` is a sync input that runs on the rate group thread.

### 3.2 I2C Address

The INA3221 I2C address is set by the A0 pin:

| A0 pin connection | 7-bit address |
|-------------------|---------------|
| GND | 0x40 |
| VS | 0x41 |
| SDA | 0x42 |
| SCL | 0x43 |

**Address TBD** — confirm A0 pin connection from PDS board schematic.

### 3.3 Register Map (relevant subset)

| Pointer | Name | Description |
|---------|------|-------------|
| 0x00 | Configuration | Reset, averaging, conversion time, operating mode |
| 0x01 | CH1 Shunt Voltage | Averaged shunt voltage, channel 1 (12 V rail) |
| 0x02 | CH1 Bus Voltage | Averaged bus voltage, channel 1 (12 V rail) |
| 0x03 | CH2 Shunt Voltage | Averaged shunt voltage, channel 2 (5 V rail) |
| 0x04 | CH2 Bus Voltage | Averaged bus voltage, channel 2 (5 V rail) |
| 0x05 | CH3 Shunt Voltage | Averaged shunt voltage, channel 3 (3.3 V rail) |
| 0x06 | CH3 Bus Voltage | Averaged bus voltage, channel 3 (3.3 V rail) |
| 0xFE | Manufacturer ID | Expected: 0x5449 (TI) |
| 0xFF | Die ID | Expected: 0x3220 |

All register values are transmitted MSB first (big-endian).

### 3.4 ADC Scaling and Current Calculation

Shunt voltage and bus voltage registers are 13-bit two's complement values in bits [15:3] (lower 3 bits unused):

| Measurement | LSB | Formula |
|-------------|-----|---------|
| Shunt voltage | 40 µV | `Vshunt_uV = (raw >> 3) × 40` |
| Bus voltage | 8 mV | `Vbus_mV = (raw >> 3) × 8` |
| Current | — | `I_mA = Vshunt_uV / Rshunt_mOhm` |

### 3.5 Channel Assignment

| Channel | PDS Rail | Shunt Resistor | Telemetry Channels |
|---------|----------|----------------|--------------------|
| CH1 | 12 V output | 25 mΩ | `CH1_VOLTAGE`, `CH1_CURRENT` |
| CH2 | 5 V output | 50 mΩ | `CH2_VOLTAGE`, `CH2_CURRENT` |
| CH3 | 3.3 V output | 36 mΩ | `CH3_VOLTAGE`, `CH3_CURRENT` |

### 3.6 Ports

| Port | Direction | Type | Purpose |
|------|-----------|------|---------|
| `schedIn` | Input | `Svc.Sched` | 1 Hz rate group tick |
| `busWrite` | Output | `Drv.I2c` | Write INA3221 registers; calls `LinuxI2cDriver.write` |
| `busWriteRead` | Output | `Drv.I2cWriteRead` | Read INA3221 registers (write register pointer then read data in one repeated-start transaction); calls `LinuxI2cDriver.writeRead` |
| `logOut` | Output | `Fw.Log` | Event logging |
| `tlmOut` | Output | `Fw.Tlm` | Six telemetry channels (CH1–CH3 voltage and current) |

### 3.7 Commands

None. `INA3221Manager` is driven entirely by the rate group tick and emits telemetry autonomously.

---

## 4. State Machine

Standard hardware manager flat F' state machine:

```
RESET (initial)
  └─ (on schedIn) [No hardware reset pin on INA3221 — no GPIO to assert.]
                  → WAIT_RESET

WAIT_RESET
  └─ (on schedIn) wait reset delay; → ENABLE

ENABLE
  └─ (on schedIn) verify I2C connectivity (read Part ID or known register); → CONFIGURE

CONFIGURE
  └─ (on schedIn) write configuration register (averaging mode, conversion time, channel enable);
                  → RUN

RUN
  └─ (on schedIn) for each channel (CH1, CH2, CH3):
                    read Vshunt register (0x01/0x03/0x05): Vshunt_uV = (raw >> 3) × 40
                    read Vbus register  (0x02/0x04/0x06): Vbus_mV   = (raw >> 3) × 8
                    compute current:    I_mA = Vshunt_uV / Rshunt_mOhm
                      CH1: Rshunt = 25 mΩ  → I_mA = Vshunt_uV / 25
                      CH2: Rshunt = 50 mΩ  → I_mA = Vshunt_uV / 50
                      CH3: Rshunt = 36 mΩ  → I_mA = Vshunt_uV / 36
                  emit six telemetry channels (CH1–CH3 voltage_mV, CH1–CH3 current_mA)

Any state: I2C error → emit WARNING_HI (throttled) → RESET
```

---

## 5. Notes

- INA3221 I2C address is 0x40–0x43 based on the A0 pin connection. Exact address TBD pending PDS schematic.
- Shunt resistor values: CH1 = 25 mΩ (12 V rail), CH2 = 50 mΩ (5 V rail), CH3 = 36 mΩ (3.3 V rail). Confirmed from hardware team.
- The INA3221 does not require a hardware reset GPIO; the RESET state simply waits a brief settling period before proceeding to ENABLE.
- Alert/limit functionality of the INA3221 (over-current flags, critical and warning thresholds) is not used in this stub implementation. It may be added in a future revision once PDS hardware is available and rail limits are characterized.
- `INA3221Manager` does not feed data to `EPSApplication`. If PDS rail health ever needs to factor into power state decisions, a `railStateOut` port to `EPSApplication` can be added at that time.
