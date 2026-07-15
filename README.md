# Automated pH System 

An Arduino system for measuring and calibrating the pH of a liquid, with automatic heating of the sample to a reference temperature and two pumps for dosing calibration solutions (base / acid). Controlled via a 4x3 matrix keypad, with status shown on a 16x2 I2C LCD.

<img width="2048" height="1536" alt="WhatsApp Image 2026-07-15 at 16 40 19" src="https://github.com/user-attachments/assets/342fd49e-e3f0-443a-b1ca-7dee0e362a3a" />

## Gallery

### Complete System

<img width="1536" height="2048" alt="WhatsApp Image 2026-07-15 at 16 40 24 (1)" src="https://github.com/user-attachments/assets/89e6d355-6b9b-4f52-a732-2e11c5984a38" />


### Electronics and Wiring

<img width="1536" height="2048" alt="WhatsApp Image 2026-07-15 at 16 40 25" src="https://github.com/user-attachments/assets/020811f9-4bda-49af-8378-9f7cc87dd96f" />


## Table of Contents

- [Project Overview](#project-overview)
- [Gallery](#gallery)
- [How It Works](#how-it-works)
- [Hardware](#hardware)
- [Pinout](#pinout)
- [Libraries](#libraries)
- [Installation](#installation)
- [Usage](#usage)
- [Code Structure](#code-structure)
- [PID Parameters](#pid-parameters)
- [Known Limitations / TODO](#known-limitations--todo)

## How It Works

The system has three main modes, selected via the keypad while a menu scrolls across the LCD:

| Key | Mode | Description |
|---|---|---|
| `1` | Calibration | Enter two known pH values (e.g. 8.3 and 2.7); the system measures the sensor voltage in both solutions and computes the line coefficients `m` and `b` (linear calibration: `pH = m*V + b`) |
| `2` | Measurement | Measures the pH sensor's voltage and converts it to a pH value using the last calibration |
| `3` | Pump 1 (base) | Runs the pump that doses the base solution for ~1.5 s |
| `4` | Pump 2 (acid) | Runs the pump that doses the acid solution for ~1.5 s |

Before every measurement or calibration, the system waits (`pertilijerWHILE`) for the sample temperature (read from the DS18B20 sensor) to reach at least 24.5 °C, since pH readings are temperature-sensitive. After that, throughout the whole measurement, the temperature is further held at 25 °C via PID control of the heater relay (`pertilijerPID`).

## Hardware

- Arduino (Uno/Nano or similar, ATmega328-class)
- 16x2 LCD with I2C backpack (address `0x27`)
- 4x3 matrix keypad
- DS18B20 digital temperature sensor (OneWire)
- pH sensor (analog output)
- Relay for the heater (Peltier/heating element)
- 2x DC pump (driven via H-bridge/relay module, 2 pins each for direction/enable)
- Power supply for the pumps (separate `ENABLE_PUMPA` pin)

## Pinout

| Pin | Function |
|---|---|
| `A0` | DS18B20 (OneWire bus, temperature) |
| `A1` | pH sensor (analog input) |
| `2` | Pump enable |
| `3` | Pump 2 (acid) — channel A |
| `1` | Pump 2 (acid) — channel B |
| `6` | Pump 1 (base) — channel A |
| `5` | Pump 1 (base) — channel B |
| `4` | Heater relay (Peltier) |
| `7,8,9,10` | Keypad rows |
| `11,12,13` | Keypad columns |
| `SDA/SCL` | LCD (I2C) |

> ⚠️ Pin `1` is the hardware RX pin for serial communication. If USB serial is used for debugging (`Serial.begin`), it will conflict with this pin — `Serial` is currently disabled in the code (commented out).

## Libraries

Install the following libraries (via the Arduino Library Manager or manually):

- `Keypad` (Mark Stanley / Alexander Brevig)
- `Wire` (built-in)
- `LiquidCrystal_I2C`
- `OneWire`
- `DallasTemperature`

## Installation

1. Install the Arduino IDE (or use PlatformIO).
2. Install the libraries listed above via **Sketch → Include Library → Manage Libraries**.
3. Wire up the hardware according to the [pinout](#pinout).
4. Open the `.ino` file, select the correct board and port.
5. Upload the code to the Arduino.

## Usage

1. On power-up, the LCD shows a scrolling menu: `Izaberi: 1-Kalibracija 2-Merenje 3-BAZA 4-KISELOST`.
2. For first use, run calibration (`1`):
   - Enter the first known pH value (integer + decimal part, `#` to confirm, `*` to delete a digit).
   - Enter the second known pH value.
   - The system waits for the temperature to reach 24.5 °C, then measures the voltage in the first solution (~120 s).
   - Move the sensor to the second solution when prompted (10 s pause for repositioning).
   - The system measures the second voltage and computes the calibration coefficients.
3. For measurement (`2`): the system waits for temperature, measures voltage (~120 s), and displays the computed pH value.
4. For dosing (`3`/`4`): activates the corresponding pump for ~1.5 seconds.

> Note: measurement durations in the code are set to 120 cycles x 1 s (~2 min) — the comments indicate it should be 300 s, but it was shortened for testing. Adjust the `br<120` conditions in `VphMerenje()` and `VphKalibracija()` as needed.

## Code Structure

```
├── setup()              → inicijalizacija LCD-a, pinova, senzora
├── loop()                → poziva odabirModa() u beskonačnoj petlji
├── odabirModa()          → glavna state-mašina, čita meni i poziva odgovarajuću funkciju
├── modovi()              → prikazuje skrolujući meni i čeka izbor korisnika
├── vodenaPumpa()         → aktivira pumpu 1 ili 2
├── pertilijerWHILE()     → čeka da temperatura dostigne minimalnu vrednost pre merenja
├── pertilijerPID()       → PID regulacija temperature tokom merenja
├── racunajPID()          → implementacija PID formule (P, I, D)
├── tempNtc()             → čita temperaturu sa DS18B20
├── VphMerenje() / VphKalibracija() → čitanje napona pH senzora tokom vremena
├── pocetnaNapon()        → inicijalno očitavanje napona
├── racunanjeM() / racunanjeB() → računanje koeficijenata kalibracione prave
├── merenjePh()           → primena kalibracije na izmereni napon → pH vrednost
├── unosCelobrojneVrednosti() → unos decimalnog broja preko tastature
└── ispisTeksta()         → skrolujući ispis teksta na LCD-u
```

## PID Parameters

| Parameter | Value | Description |
|---|---|---|
| `SETPOINT` | 25.0 °C | Target sample temperature |
| `MRTVA_ZONA` | 0.5 | Dead-zone threshold below which the relay stays off |
| `Kp` | 2.0 | Proportional coefficient |
| `Ki` | 0.05 | Integral coefficient |
| `Kd` | 1.0 | Derivative coefficient |
| `PID_PERIOD` | 1000 ms | PID computation period |

The relay is used as an on/off output (no PWM control of the heater), so the PID output only acts as a threshold: above `MRTVA_ZONA` the relay turns on, otherwise it stays off.

## Known Limitations / TODO

- No check for `DEVICE_DISCONNECTED_C` (disconnected temperature sensor) — the check exists in the code but is commented out.
- `Serial` debugging is disabled; all output goes to the LCD only.
- Audio feedback (`zvucniSignal()`) is implemented but commented out throughout the code.
- Measurement duration is set to ~2 min instead of the intended 5 min (a code comment indicates this is temporary, for testing).
- The file contains a fair amount of commented-out legacy code (an older `loop()` without the `odabirModa()` function) that could be removed for readability.
