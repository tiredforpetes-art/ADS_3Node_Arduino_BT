#include "protocol.h"

// CRC 8bit simple (poly 0x07)
uint8_t crc8(const uint8_t *data, size_t len){
  uint8_t crc = 0x00;

  for(size_t i=0;i<len;i++){
    crc ^= data[i];
    for(int j=0;j<8;j++){
      if(crc & 0x80) crc = (crc<<1) ^ 0x07;
      else crc <<=1;
    }
  }
  return crc;
}

// FRAME V2
// [START][TYPE][TIMESTAMP 4][CURRENT float 4][CRC]
size_t buildCurrentFrame(uint8_t* frame, uint32_t timestamp_ms, float current){

  frame[0] = 0xAA;
  frame[1] = 0x01; // TYPE: current sensor

  memcpy(frame+2, &timestamp_ms, 4);
  memcpy(frame+6, &current, 4);

  frame[10] = crc8(frame+1, 9);

  return 11;
}
