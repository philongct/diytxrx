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
#include "logger.h"
#include "serialcommand.h"

// RF packet
uint8_t dataPacket[PACKET_LEN];

//// Read commands from serial
//SerialCommand serialCommand;

// Notifier system
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
  printf_begin();
  printlog(1, "Initiallizing...");
  
  notifier.begin();
  tx.begin();
  GLOBAL_CFG.load();

  if (input.calibrateGimbalMidPoint(&GLOBAL_CFG)) {
    GLOBAL_CFG.save();
  }
  
  printlog(1, "Starting now");
  GLOBAL_CFG.show(1);
//  tx.sync();
}

// the loop routine runs over and over again forever:
void loop() {
  notifier.loop();

  runLoop();
}

int16_t remap(uint16_t input) {
  int16_t val = map(input, 0, 1023, 1000, 2000);
  if (GLOBAL_CFG.midPointCorrection && val < 1503 && val > 1497) val = 1500;

  return val;
}

void runLoop() {
  bool changed = input.readAnalog();
  if (changed) {
    sbus.setChannelData(0, map(input.analogVals[0], 0, 1023, 1000, 2000));
    sbus.setChannelData(1, remap(input.analogVals[1] - GLOBAL_CFG.gimbalMidPointsDelta[0]));
    sbus.setChannelData(2, remap(input.analogVals[2] - GLOBAL_CFG.gimbalMidPointsDelta[1]));
    sbus.setChannelData(3, remap(input.analogVals[3] - GLOBAL_CFG.gimbalMidPointsDelta[2]));

//    // TODO: remove or configured whether print or not
//    printlog(2, ">>gas:%d,yaw:%d,roll:%d,pitch:%d", sbus.getChannelData(0), sbus.getChannelData(1), sbus.getChannelData(2), sbus.getChannelData(3));
  }

//  return; // Print analog only

  if (input.readDigital()) {
    sbus.setChannelData(4, input.currentFlightMode);
    sbus.setChannelData(6, input.aux[0]);
    sbus.setChannelData(7, input.aux[1]);
    sbus.setChannelData(8, input.aux[2]);

    changed = true;
  }

  sbus.buildPacket(dataPacket, HEADER_OFFSET);
  if (millis() - lastSend >= 500 || changed) {
    tx.buildDataPacket(dataPacket);
    if (tx.transmitPacket(&dataPacket)) {
      lastSend = millis();
      if (tx.receiveUntilTimeout(&dataPacket, 50)) {
        lostCount = 0;
        printlog(3, "Data: ");
        for (uint8_t i = 0; i < 25; ++i) {
          printlog(3, "%d", dataPacket[HEADER_OFFSET + i]);
        }
      } else {
        ++lostCount;
      }
    } else {
      tx.sync();
    }
  }

  if (lostCount > 3 ) {
    notifier.warnRf(1);
  } else {
    notifier.showOK();
  }
}


