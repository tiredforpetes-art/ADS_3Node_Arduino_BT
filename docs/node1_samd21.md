# Node 1 (XIAO SAMD21) — Current Acquisition + SD Logging

## Firmware location
`firmware/node1_samd21_current_acquisition/src/`

## What it does
- Reads current using an ACS725 module on **A0**.
- Converts ADC reading to amperes.
- Logs averaged data to microSD.
- Sends UART frames (Frame V2) through `Serial1`.

## Notes
- `Serial1` pins on XIAO SAMD21 are fixed by the board variant:
  - TX: **D6**
  - RX: **D7**
- Avoid blocking on USB Serial at boot if you need battery-only operation.
