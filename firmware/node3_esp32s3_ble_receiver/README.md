# Node 3 — BLE Receiver + OLED + microSD Logger (XIAO ESP32-S3)

This folder contains the firmware for **Node 3** of the ADS 3-node system.

## Role in the system

Node 3 is the **BLE receiver** in the architecture:

- **Node 1 (XIAO SAMD21):** current acquisition
- **Node 2 (XIAO ESP32-S3):** receives frames over UART and forwards them over BLE
- **Node 3 (XIAO ESP32-S3):** subscribes to BLE notifications, decodes frames, shows status on OLED, and logs received data to microSD

## Stable features in this version

- BLE client for Node 2 notifications
- Optional OLED status display
- microSD logging on the XIAO Expansion Board
- FIFO queue between BLE callback and main loop
- Debug output over Serial
- CSV logging with automatic file numbering

## Files

- `src/daq_node3_BT_ESP32S3.ino` — main firmware
- `src/display_manager.h/.cpp` — OLED helper
- `src/logger.h/.cpp` — SD logger

## BLE protocol

Node 3 expects the same 11-byte frame used by the ADS setup:

`[START][TYPE][TIMESTAMP 4B][CURRENT float 4B][CRC/last byte reserved]`

Current decoding uses:

- `START = 0xAA`
- `TYPE = 0x01`

## BLE UUIDs

- **Service UUID:** `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- **Characteristic UUID:** `6e400003-b5a3-f393-e0a9-e50e24dcca9e`

## Hardware

- Seeed Studio **XIAO ESP32-S3**
- Seeed Studio **XIAO Expansion Board**
- Optional **SSD1306 OLED** over I2C
- microSD card in the Expansion Board slot

## Pin usage

### OLED (I2C)

- `D4` → SDA
- `D5` → SCL

### microSD (SPI)

- `D8` → SCK
- `D9` → MISO
- `D10` → MOSI
- `D2` → CS

> Important: for the XIAO Expansion Board microSD slot, Node 3 uses `SPI.begin(D8, D9, D10, D2)` and the correct CS is **D2**.

## CSV output format

Each received frame is logged as:

`Time_s ; Current_A ; Unit`

Example:

```csv
Time_s ; Current_A ; Unit
61.951 ; 0.0217 ; A
62.437 ; 0.0184 ; A
62.679 ; 0.0184 ; A
```

## Arduino libraries required

- `NimBLE-Arduino`
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `SD`
- `SPI`

## Board configuration

In Arduino IDE:

- **Board:** Seeed XIAO ESP32S3
- Use the ESP32 board package version already working in your Node 2 / Node 3 setup

## How it works

1. Node 3 starts Serial, optional OLED, and SD logging.
2. It scans for Node 2 BLE advertising.
3. Once connected, it subscribes to notifications.
4. The BLE callback pushes frames into a RAM queue.
5. The main loop dequeues, decodes, updates the display, and logs to CSV.

This design avoids writing to SD directly inside the BLE callback.

## Notes

- This is the **stable working version** currently kept for Node 3.
- A more advanced reconnect/session-splitting version was tested separately, but this stable version is the current baseline.
- The logger keeps the file open and flushes periodically for more reliable logging.
