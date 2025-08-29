# ESP32 Composite Video Dashboard

A digital dashboard for a car, displaying speed, coolant temperature, fuel level, and oil status on a composite video display.

## Features

- Speedometer
- Coolant temperature gauge
- Fuel level gauge
- Oil status indicator

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

## Serial Communication

The dashboard receives data from the serial port.

The message format is `KEY:VALUE`, where `KEY` can be `SPEED`, `OIL`, `COOLANT`, or `FUEL`.

**Example:**

```
SPEED:120
OIL:0
COOLANT:90
FUEL:50
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
