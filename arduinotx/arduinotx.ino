/*
 * 
 * A1-A4: Analog
 * D2: left button (flight mode)
 * D3: right button (AUX selection)
 * D4-D5: AUX up/down
 * D6-D7: LED status (flight mode/radio signal)
 * D8: Buzzer
 * 
 * SPI pins:
 * D9: CE
 * D10: CSN
 * D11: MOSI
 * D12: MISO
 * D13: SCK
 * 
 */

//#include <SPI.h>


#include "SBUS.h"
#include "tx.h"
#include "output.h"
#include "input.h"

#define LED           6

#define RF_CE_PIN     9
#define RF_CSN_PIN    10

// RF packet
uint8_t dataPacket[PACKET_LEN];

// Input
Input input;

// RF Transmitter
TX tx(RF_CE_PIN, RF_CSN_PIN);

// SBUS data builder
SBUS sbus;

void setup(){
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  tx.begin();
}

// the loop routine runs over and over again forever:
void loop() {
  if (input.readAnalog()) {
    sbus.setChannelData(0, input.analogVals[0]);
    sbus.setChannelData(1, input.analogVals[1]);
    sbus.setChannelData(2, input.analogVals[2]);
    sbus.setChannelData(3, input.analogVals[3]);
  }
  
  sbus.buildPacket(dataPacket, HEADER_OFFSET);

  if (tx.transmitPacket(&dataPacket)) {
    if (tx.receiveUntilTimeout(&dataPacket)) {
      Serial.println("Data 0: ");
      for (uint8_t i = 0; i < 25; ++i) {
        Serial.println(dataPacket[HEADER_OFFSET + i]);
      }
    }
  } else {
    tx.sync();
  }
  
  delay(1000); // wait a second and do it again.
//  tx.testSignal();
}
