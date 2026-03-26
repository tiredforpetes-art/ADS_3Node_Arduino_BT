# Node 1 – SAMD21 Current Acquisition

## Overview

Node 1 is the data acquisition unit of the system.

It is responsible for measuring electrical current using an analog sensor, processing the signal, and generating structured data frames for transmission.

The node is implemented on a Seeed Studio XIAO SAMD21 and operates as the primary data source in the acquisition chain.

---

## Functional Role

Node 1 performs the following tasks:

1. Acquire analog signal from current sensor (ACS725)
2. Convert ADC readings into calibrated current values
3. Apply signal filtering (moving average)
4. Detect sensor activity (idle vs active)
5. Generate structured binary frames
6. Transmit frames via UART to Node 2
7. Log measurements to microSD card

---

## Hardware

### Board

Seeed Studio XIAO SAMD21

### Sensor

ACS725 Current Sensor (10A version)

Powered at:

3.3V

---

## Analog Acquisition

### ADC Configuration

- Resolution: 12-bit
- Reference voltage: 3.3V

ADC range:

0 → 4095

---

### Signal Conversion

Measured voltage is converted to current using:

V = (ADC / 4095) * 3.3  
I = (V - V_OFFSET) / sensitivity

Where:

- V_OFFSET ≈ 0.3V (calibrated)
- sensitivity depends on ACS725 model

---

## Signal Filtering

A moving average filter is applied:

Window size: 10 samples

Purpose:

- reduce noise
- stabilize readings
- improve measurement reliability

---

## Sampling and Logging Strategy

### Sampling interval

~1 second

### Logging interval

~10 seconds

The node computes the average of the collected samples and stores it.

---

## microSD Logging

Library: SD.h (SPI)

Chip Select (CS): D2

File format:

Tiempo(s) ; Data point ; Data unit

Example:

10.123 ; 0.4374 ; A  
20.076 ; 0.4818 ; A  

---

## UART Communication

Pins:

TX → D6  
RX → D7  

Configuration:

Baudrate: 115200  
Format: 8N1  

---

## Data Frame Structure

11 bytes

| Byte | Field |
|-----|------|
| 0 | Start byte (0xAA) |
| 1 | Frame type |
| 2–5 | Timestamp (uint32) |
| 6–9 | Current value (float) |
| 10 | CRC8 |

---

## CRC Validation

CRC8 (poly 0x07) ensures data integrity.

---

## Activity Detection

IDLE_THRESHOLD ≈ 0.002 A

---

## System Role

Sensor → Node1 → UART → Node2 → BLE → Node3

---

## Future Improvements

- dynamic calibration
- higher sampling frequency
- DMA-based ADC reading
- error logging and fault detection
