
//#define __DEBUG__

// Turn this on for debug log to serial

/*
 * 
 * inputs:
 * A1-A4: Analog (battery monitor)
 * D2-D3: sbus communication
 * 
 * SPI pins:
 * D5: CE
 * D6: CSN
 * D11: MOSI
 * D12: MISO
 * D13: SCK
 */

#define SBUS_TX         2
#define SBUS_BAUDRATE   100000

#define SBUS_DATA_LEN   25

// Lost if not receiving any signal after 1s
#define RADIO_LOST_TIMEOUT  1500

#include "input.h"
#include "logger.h"
#include "twowaysync_protocol.h"
#include "serial_writer.h"

Input input;

// Soft serial to write data out
SerialWriter invertedConn(SBUS_TX, true);

TwoWaySyncProtocol rx;

// RF packet buffer
uint8_t sbusPacket[SBUS_DATA_LEN] = {   // 11 channels available
  0x0f,0x01,0x04,0x20,0x00,0xff,0x07,0x40,0x00,0x02,0x10,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

int32_t delayTime;

void setup(){
  printf_begin();
  delay(500);
  invertedConn.begin(SBUS_BAUDRATE);
  input.begin();

  rx.init();
}

// the loop routine runs over and over again forever:
void loop() {
  while(micros() < delayTime); // busy wait
  if (rx.receiveData(&delayTime)) {
    rx.buildSbusPacket(sbusPacket);
  }

  u16 arr[] = {rx.stats.lqi, rx.stats.rssi};
  extendSbusPacket(&sbusPacket[1], 15, 1, 11*13, arr); // extends to 2 channels including lqi & rssi
  sbusLostSignal(sbusPacket);
  sbusWrite(sbusPacket);

  rx.stats.battery = min(input.getCellVoltage(1), input.getCellVoltage(2));
  // increase cycleCount
  if (rx.stats.cycleCount++ % 100 == 0) {
    printlog(0, "--> lqi %d", rx.stats.lqi);
  }
}

// byteNo = 0 to n
// bitNo = 0 to 7 (right to left)
void extendSbusPacket(uint8_t* sbus_data_pkt, u8 byteNo, u8 bitNo, u8 maxBit, u16* extendedData) {
  // reset counters
  uint8_t ch = 0;
  uint8_t bit_in_channel = 0;
  uint8_t byte_in_sbus = byteNo;
  uint8_t bit_in_sbus = bitNo;

  // store servo data
  for (u8 i = 8 * byteNo + bitNo; i < maxBit; i++) {
    if (extendedData[ch] & (1 << bit_in_channel)) {
      sbus_data_pkt[byte_in_sbus] |= (1 << bit_in_sbus);
    }
    
    bit_in_sbus++;
    bit_in_channel++;

    if (bit_in_sbus == 8) {
      bit_in_sbus = 0;
      byte_in_sbus++;
      sbus_data_pkt[byte_in_sbus] = 0;
    }
    if (bit_in_channel == 11) {
      bit_in_channel = 0;
      ch++;
    }
  }
}

void sbusLostSignal(uint8_t *sbusPacket) {
  sbusPacket[23] = 0;
  if (rx.isRadioLost() && input.getfailSafeEnabled()) {
    sbusPacket[23] |= (1<<2);   // frame lost
    sbusPacket[23] |= (1<<3);   // activate failsafe
    Serial.println("failsafe on");
  }
}

// SBUS interval is 14ms. Current implementation already creates 14ms interval
void sbusWrite(uint8_t *sbusPacket) {
  for (uint8_t i = 0; i < SBUS_DATA_LEN; ++i) {
    invertedConn.write(sbusPacket[i], true, true);
  }
}

