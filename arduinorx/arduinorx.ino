
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

// Lost if not receiving any signal after 1s
#define RADIO_LOST_TIMEOUT  10000

#include "rx.h"
#include "SoftSerial.h"

// Soft serial to write data out
SoftSerial invertedConn(SBUS_RX, SBUS_TX, true);

// Radio rx
RX rx(RF_CE, RF_CSN);

// RF packet buffer
uint8_t dataPacket[PACKET_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0f,0x01,0x04,0x20,0x00,0xff,0x07,0x40,0x00,0x02,0x10,0x80,0x2c,0x64,0x21,0x0b,0x59,0x08,0x40,0x00,0x02,0x10,0x80,0x00,0x00};

long lastWrite = millis();
long lastReceived = millis();
bool synced = false;

void setup(){
  Serial.begin(115200);
  delay(2000); // For programming
  invertedConn.begin(SBUS_BAUDRATE);
  rx.begin();
}

// the loop routine runs over and over again forever:
void loop() {
  if (rx.receive(&dataPacket)) {
    Serial.println("Packet received");
    lastReceived = millis();
    if (dataPacket[0] == SYNC_PACKET) {
      rx.syncResponse(10);
      Serial.println("Responsed to sync packet");
    } else if (dataPacket[0] == SYNC_TEST_PACKET) {
      rx.syncTestResponse();
      synced = true;
      Serial.println("Responsed to sync test packet");
    } else {
      rx.transmitPacket(&dataPacket);
    }
  } else {
    if (synced && millis() - lastReceived > RADIO_LOST_TIMEOUT) {
//      Serial.println("Signal lost");
      sbusLostSignal(dataPacket);
    }
  }
  
  sbusWrite(dataPacket);
}

void sbusLostSignal(uint8_t *packet) {
  packet[HEADER_OFFSET + 23] |= (1<<2);
}

void sbusWrite(uint8_t *data) {
  if (millis() - lastWrite >= 14) {
    for (uint8_t i = 0; i < 25; ++i) {
      invertedConn.write(data[HEADER_OFFSET + i], true, true);
    }
    lastWrite = millis();
  }
}

