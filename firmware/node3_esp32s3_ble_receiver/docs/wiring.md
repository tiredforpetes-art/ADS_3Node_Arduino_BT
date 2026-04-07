# Node 3 wiring notes

## Board
- Seeed Studio XIAO ESP32-S3
- Seeed Studio XIAO Expansion Board

## OLED
- SDA: D4
- SCL: D5
- Address: 0x3C

## microSD
- SCK: D8
- MISO: D9
- MOSI: D10
- CS: D2

## Important SD note
The microSD slot on the XIAO Expansion Board works correctly in this firmware with:

```cpp
SPI.begin(D8, D9, D10, D2);
SD.begin(D2, SPI);
```

Using the wrong CS pin or relying on a default SPI init can prevent logging or create unstable behavior.
