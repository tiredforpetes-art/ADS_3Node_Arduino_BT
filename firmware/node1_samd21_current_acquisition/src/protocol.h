#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

size_t buildCurrentFrame (uint8_t* frame, uint32_t timestamp, float current);
uint8_t crc8(const uint8_t*data, size_t len);

#endif