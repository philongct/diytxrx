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

#include "config.h"

#include "SBUS.h"
#include "tx.h"
#include "notifier.h"
#include "input.h"

// RF packet
uint8_t dataPacket[PACKET_LEN];

Notifier notifier;

// Input
Input input(&notifier);

// RF Transmitter
TX tx(RF_CE_PIN, RF_CSN_PIN);

// SBUS data builder
SBUS sbus;

// always lost until RF is connected
volatile uint16_t lostCount = 10;

long lastSend = millis();

void setup(){
  Serial.begin(115200);
  notifier.begin();
  tx.begin();
  Serial.println("Begin");
//  tx.sync();
}

// the loop routine runs over and over again forever:
void loop() {
  notifier.loop();

//  outputDebug();
  runLoop();
}

//void outputDebug() {
//  if (Serial.available() > 0) {
//    int inChar = Serial.read();
//    Serial.println("Read: " + String(inChar));
//    if (inChar == 'a') {
//      notifier.warnRf();
//    } else if (inChar == 'b'){
//      notifier.showFlightMode();
//    } else {
//    }
//  }
//}

void runLoop() {
  bool changed = false;
  if (input.readAnalog()) {
    sbus.setChannelData(0, input.analogVals[0]);
    sbus.setChannelData(1, input.analogVals[1]);
    sbus.setChannelData(2, input.analogVals[2]);
    sbus.setChannelData(3, input.analogVals[3]);

    changed = true;
  }
  
  sbus.buildPacket(dataPacket, HEADER_OFFSET);
  if (millis() - lastSend >= 1000 || changed) {
    tx.buildDataPacket(dataPacket);
    if (tx.transmitPacket(&dataPacket)) {
      lastSend = millis();
      if (tx.receiveUntilTimeout(&dataPacket)) {
        lostCount = 0;
        Serial.println("\nData 0: ");
        for (uint8_t i = 0; i < 25; ++i) {
          Serial.print(dataPacket[HEADER_OFFSET + i]);Serial.print(" ");
        }
      } else {
        ++lostCount;
      }
    } else {
      tx.sync();
    }
  }

  if (lostCount > 4 ) {
    notifier.warnRf();
  } else {
    notifier.showFlightMode();
  }
}


