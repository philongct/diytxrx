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
#define SBUS_RX         3
#define SBUS_BAUDRATE   100000

#define RF_CE     4
#define RF_CSN    7

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

void setup(){
  Serial.begin(115200);
  delay(3000); // For programming
  invertedConn.begin(SBUS_BAUDRATE);
  rx.begin();
}

// the loop routine runs over and over again forever:
void loop() {
  if (rx.receive(&dataPacket)) {
    Serial.println("Packet received");
    if (dataPacket[0] == SYNC_PACKET) {
      rx.syncResponse(10);
      Serial.println("Responsed to sync packet");
    } else if (dataPacket[0] == SYNC_TEST_PACKET) {
      rx.syncTestResponse();
      Serial.println("Responsed to sync test packet");
    } else {
      rx.transmitPacket(&dataPacket);
      Serial.println("Packet responsed");
    }
  }

  sbusWrite(dataPacket);
}

void sbusWrite(uint8_t *data) {
  if (millis() - lastWrite >= 14) {
    for (uint8_t i = 0; i < 25; ++i) {
      invertedConn.write(data[HEADER_OFFSET + i], true, true);
//      Serial.println(data[HEADER_OFFSET + i]);
    }
    lastWrite = millis();
  }
}

