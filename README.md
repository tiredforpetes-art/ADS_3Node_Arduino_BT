# ADS_3Node_Arduino_BT

A 3-node data acquisition system built around Seeed Studio XIAO boards and the XIAO Expansion Board.

## Nodes

### Node 1 — Current Acquisition + SD Logging (XIAO SAMD21)
- Reads current from an **ACS725** sensor via ADC.
- Applies a moving-average filter.
- Logs averaged values to microSD (CSV).
- Transmits measurement frames over **UART** (`Serial1`).

### Node 2 — UART Receiver + BLE Bridge (XIAO ESP32-S3)
- Receives UART frames from Node 1.
- Validates CRC and counts frames.
- Displays status on OLED.
- Forwards frames over **BLE notifications** to Node 3.

### Node 3 — BLE Receiver (XIAO ESP32-S3)
- BLE client.
- Subscribes to Node 2 notifications.
- Decodes frames and post-processes the data.

> Node 2 and Node 3 firmware will be added next.

## Repository layout

- `firmware/` — Arduino code per node
- `docs/` — wiring, protocol, and architecture documentation

## Quick start (Node 1)

1. Open `firmware/node1_samd21_current_acquisition/src/node1_samd21_current_acquisition.ino` in Arduino IDE.
2. Select your board: **Seeed XIAO SAMD21**.
3. Upload.
4. See `docs/wiring.md` for sensor, SD, and UART wiring notes.

## Important note: UART on XIAO Expansion Board

The UART JST header on the XIAO Expansion Board is labeled `GND / 3V3 / TX-D6 / RX-D7`.

A **straight JST-to-JST cable does not cross TX/RX** (TX connects to TX and RX to RX). UART requires **TX ↔ RX crossing**, so use dupont wires or a crossed cable.

## License

Add a license if/when you decide how you want this project to be shared.
