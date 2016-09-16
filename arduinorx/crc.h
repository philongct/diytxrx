#ifndef __CRC_H__
#define __CRC_H__

const uint16_t CRC_Short[]={
  0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
  0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7 };


static uint16_t CRCTable(uint8_t val)
{
  uint16_t word ;
  word = CRC_Short[val&0x0F] ;
  val /= 16 ;
  return word ^ (0x1081 * val) ;
}

static uint16_t crc16(uint8_t *data, uint8_t len)
{
  uint16_t crc = 0;
  for(uint8_t i=0; i < len; i++)
    crc = (crc<<8) ^ CRCTable((uint8_t)(crc>>8) ^ *data++);
  return crc;
}

#endif
