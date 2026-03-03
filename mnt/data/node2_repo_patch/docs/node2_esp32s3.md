# Node 2 — XIAO ESP32-S3 (UART → BLE Bridge)

## Purpose
Node 2 receives measurement frames from **Node 1 (XIAO SAMD21)** over UART, validates them using **CRC8**, shows runtime status on the **OLED** (XIAO Expansion Board), and forwards the raw frames to **Node 3** via **BLE notifications**.

## Hardware
- Board: Seeed Studio **XIAO ESP32-S3**
- Carrier: **XIAO Expansion Board v1.1**
- OLED: SSD1306 128×64 over I2C (typically address `0x3C`)

## UART wiring (from Node 1)
Node 2 expects UART on the Expansion Board UART pins:
- **RX = D7**
- **TX = D6** (optional)

Minimum connections:
- **Node1 TX (D6) → Node2 RX (D7)**
- **GND ↔ GND**

> Note: a straight JST-to-JST UART cable between two Expansion Boards is not cross-wired (TX→TX, RX→RX). Use dupont wiring or a crossed cable.

## UART settings
- 115200 baud, 8N1

## BLE
- Device name (advertised): `NODE2_BRIDGE`
- Service UUID: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- Notify characteristic UUID: `6e400003-b5a3-f393-e0a9-e50e24dcca9e`

Node 2 forwards the **raw 11-byte UART frame** as the BLE notification payload.

## Dependencies (Arduino IDE)
Install these libraries:
- `NimBLE-Arduino`
- `Adafruit GFX Library`
- `Adafruit SSD1306`

## Output on OLED
The OLED shows:
- Frames OK (valid CRC)
- CRC errors
- BLE connection status
- Last current (A) and timestamp (s)
