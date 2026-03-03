# System Architecture

## Data flow

ACS725 sensor → Node 1 (SAMD21 ADC + filtering) → UART Frame V2 (CRC) → Node 2 (ESP32-S3 bridge) → BLE notifications → Node 3 (ESP32-S3 receiver)

## Node responsibilities

### Node 1 (XIAO SAMD21)
- Read and convert analog sensor output to current (A)
- Filter and average samples
- Log to microSD (CSV)
- Send binary frames over UART

### Node 2 (XIAO ESP32-S3)
- Parse UART stream, resync on START+CRC
- Maintain frame counters and last value
- Display status on OLED
- Forward frames via BLE notifications

### Node 3 (XIAO ESP32-S3)
- Connect to Node 2 BLE service
- Subscribe to notifications
- Decode frames and post-process
