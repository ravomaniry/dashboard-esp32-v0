# ESP32 Composite Video Dashboard

A digital dashboard for a car, displaying speed, coolant temperature, fuel level, and oil status on a composite video display.

## Features

- Speedometer
- Coolant temperature gauge
- Fuel level gauge
- Oil status indicator
- Glow plug indicator
- Semi-circular gauges around the speedometer

## Hardware

- ESP32 Dev Module

## Software

- Arduino Framework
- PlatformIO
- [ESP32CompositeColorVideo](https://github.com/marciot/ESP32CompositeColorVideo) library

## Setup

1. Clone the repository.
2. Install [PlatformIO](https://platformio.org/).
3. Connect the ESP32 to your computer.
4. Build and upload the project using the `/home/ravo/.platformio/penv/bin/pio run --target upload` command.

## Demo Mode

Set the `IS_DEMO` variable in `src/main.ino` to `true` to enable a demo mode that cycles through various values. This is useful for testing the display without a serial connection.

## Serial Communication

The dashboard receives data from the serial port.

The message format is `KEY:VALUE`, where `KEY` can be `SPEED`, `OIL`, `COOLANT`, `FUEL`, or `GLOW`.

**Example:**

```
SPEED:120
OIL:0
COOLANT:90
FUEL:50
GLOW:1
```

WIRING:

```
Arduino TX (5V) ──470Ω──┐── ESP32 RX
                          │
                          1kΩ
                          │
                         GND

ESP32 TX ────────────────> Arduino RX  (direct connection)
GND ───────────────────── GND
```

**Note:** The ESP32 operates at a 3.3V logic level. The provided wiring diagram includes a voltage divider to protect the ESP32's 3.3V RX pin from the Arduino's 5V TX signal. While the direct connection from the ESP32's 3.3V TX to the Arduino's 5V RX might work, a logic level shifter is recommended for reliable communication.
