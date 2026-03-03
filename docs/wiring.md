# Wiring

## Node 1 (XIAO SAMD21)

### ACS725 current sensor (example)
- Power the module from **3.3V** (recommended with SAMD21 ADC).
- Connect sensor output to **A0**.
- Ensure a common **GND**.

> The ACS725 module sensitivity depends on the exact variant. The firmware uses a conversion in `sensor.cpp`.

### microSD
- Connect microSD via SPI as required by your hardware.
- Set the correct SD **CS pin** in firmware (`SD_CS_PIN`).

### UART to Node 2
Minimum required (one-way):
- **GND ↔ GND**
- **Node 1 TX (D6 / Serial1 TX) → Node 2 RX (D7)**

Optional (two-way):
- **Node 2 TX (D6) → Node 1 RX (D7)**

## Important note: XIAO Expansion Board UART JST header
The UART JST header is labeled `GND / 3V3 / TX-D6 / RX-D7`.

A straight JST-to-JST cable does **not** cross TX/RX. UART requires **TX ↔ RX** crossing.
Use dupont wires or a crossed cable.
