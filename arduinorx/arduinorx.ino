
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

#define RF_CE           4
#define RF_CSN          7

#define SBUS_TX         2
#define SBUS_RX         3
#define SBUS_BAUDRATE   100000

#define SBUS_DATA_LEN   25
#define SBUS_DELAY      14

#define NO_SYNCED       254

// Lost if not receiving any signal after 1s
#define RADIO_LOST_TIMEOUT  10000

#include "rx.h"
#include "SoftSerial.h"

// Soft serial to write data out
SoftSerial invertedConn(SBUS_RX, SBUS_TX, true);

// Radio rx
RX rx(RF_CE, RF_CSN);

// RF packet buffer
uint8_t sbusPacket[SBUS_DATA_LEN] = {
  0x0f,0x01,0x04,0x20,0x00,0xff,0x07,0x40,0x00,0x02,0x10,0x80,0x2c,0x64,0x21,0x0b,0x59,0x08,0x40,0x00,0x02,0x10,0x80,0x00,0x00};

long lastWrite = millis();
long lastReceived = millis();

uint8_t syncedCh = NO_SYNCED;

void setup(){
  Serial.begin(115200);
  delay(500);
  invertedConn.begin(SBUS_BAUDRATE);
  rx.begin();
}

bool validatePacket(uint8_t *packet) {
  return packet[1] == STAMP;
}

// the loop routine runs over and over again forever:
void loop() {
  static uint8_t dataPacket[PACKET_LEN];
  dataPacket[0] = 0x00;
  dataPacket[1] = 0x00;

  // ok variable to verify content
  // in case of noise
  bool ok = false;
  if (rx.receive(&dataPacket)) {
    Serial.print("Packet received ");Serial.println(dataPacket[0]);
    ok = validatePacket(dataPacket);
    
    if (dataPacket[0] == SYNC_PACKET) {
      if (syncedCh > AVAILABLE_CH || dataPacket[HEADER_OFFSET] == SYNC_PACKET)
        syncedCh = rx.testSignal();
      rx.syncResponse(syncedCh);
      Serial.println("Responsed to sync packet channel " + String(syncedCh));
    } else if (dataPacket[0] == SYNC_TEST_PACKET) {
      rx.syncTestResponse();
      Serial.println("Responsed to sync test packet and set new channel");
      if ( syncedCh < AVAILABLE_CH) {
        rx.setWorkingCh(syncedCh);
      }
    } else {
      rx.transmitPacket(&dataPacket);
    }
  }

  // copy sbus data in data packet
  if (dataPacket[0] == DATA_PACKET) {
    memcpy(sbusPacket, &dataPacket[HEADER_OFFSET], SBUS_DATA_LEN);
//    sbusPrint(sbusPacket);
  }

  if (ok) lastReceived = millis();
  if (syncedCh != NO_SYNCED && millis() - lastReceived > RADIO_LOST_TIMEOUT) {
//      Serial.println("Signal lost");
      sbusLostSignal(sbusPacket);
    }
  
  sbusWrite(sbusPacket);
}

void sbusLostSignal(uint8_t *sbusPacket) {
  sbusPacket[23] |= (1<<2);
}

//void sbusPrint(uint8_t *sbusPacket) {
//    for (uint8_t i = 0; i < SBUS_DATA_LEN; ++i) {
//      Serial.print(sbusPacket[i]);Serial.print(" ");
//    }
//    Serial.println("");
//}

void sbusWrite(uint8_t *sbusPacket) {
  if (millis() - lastWrite >= SBUS_DELAY) {
    for (uint8_t i = 0; i < SBUS_DATA_LEN; ++i) {
      invertedConn.write(sbusPacket[i], true, true);
    }
    lastWrite = millis();
  }
}

