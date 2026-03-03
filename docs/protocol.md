# UART Protocol (Frame V2)

Node 1 transmits a fixed-length binary frame over UART. Node 2 validates CRC and forwards the raw frame over BLE.

## UART settings
- **Baud:** 115200
- **Format:** 8N1

## Frame format
**Length:** 11 bytes

| Byte(s) | Field      | Type                         | Notes |
|--------:|------------|------------------------------|------|
| 0       | START      | `uint8`                      | `0xAA` |
| 1       | TYPE       | `uint8`                      | `0x01` = current measurement |
| 2..5    | TIMESTAMP  | `uint32` little-endian       | milliseconds since boot |
| 6..9    | CURRENT    | `float32` IEEE754 little-endian | current in amperes |
| 10      | CRC8       | `uint8`                      | CRC8 over bytes 1..9 |

## CRC8
- **Polynomial:** `0x07`
- **Init:** `0x00`
- Computed over `frame[1..9]` (TYPE + payload):

`crc = crc8(frame + 1, 9)`
