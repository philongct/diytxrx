
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

u32 loopCounter = 0;
int32_t delayTime;

void setup(){
  printf_begin();
  delay(500);
  invertedConn.begin(SBUS_BAUDRATE);
  input.begin();

  initTimer1();

  rx.init();
}

void initTimer1() {
  // initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  TCCR1B |= (1 << CS11);    // 8 prescaler (0.5 uS Time per counter tick)
  interrupts();
}

// the loop routine runs over and over again forever:
void loop() {
  while((u32)TCNT1 * 2 < delayTime && delayTime != 0); // busy wait
  
  bool received = rx.receiveData(&delayTime);
  TCNT1 = 0;
  if (received) {
    rx.buildSbusPacket(sbusPacket);
  }

  if (rx.isRadioLost() && input.getfailSafeEnabled()) {
    sbusLostSignal(sbusPacket);
  }

  sbusWrite(sbusPacket);

  rx.stats.battery1 = input.getCellVoltage(1);
  rx.stats.battery1 = input.getCellVoltage(2);
  if (loopCounter++ % 100 == 0) {
    printlog(0, "--> lqi %d", rx.stats.lqi);
  }

//  Serial.print("r ");
//  Serial.println(TCNT1);
}

void sbusLostSignal(uint8_t *sbusPacket) {
  sbusPacket[23] |= (1<<2);
}

// SBUS interval is 14ms. Current implementation already creates 14ms interval
void sbusWrite(uint8_t *sbusPacket) {
  for (uint8_t i = 0; i < SBUS_DATA_LEN; ++i) {
    invertedConn.write(sbusPacket[i], true, true);
  }
}

