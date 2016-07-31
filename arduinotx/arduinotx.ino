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
 * D9: RF24_CE/CSN2
 * D10: CSN
 * D11: MOSI
 * D12: MISO
 * D13: SCK
 * 
 */

#include "module_config.h"
#include "config.h"

#include "protocol.h"
#include "notifier.h"
#include "input.h"
#include "logger.h"
#include "serialcommand.h"

#include "twowaysync_protocol.h"

//// Read commands from serial
//SerialCommand serialCommand;

// Notifier system
Notifier notifier;

// Input
Input input(&notifier);

TwoWaySyncProtocol cur_protocol;

void setup(){
  printf_begin();
  printlog(1, "Initiallizing...");
  
  notifier.begin();
  GLOBAL_CFG.load();

  if (input.calibrateGimbalMidPoint(&GLOBAL_CFG)) {
    GLOBAL_CFG.save();
  }
  
  printlog(1, "Starting now");
  GLOBAL_CFG.show(1);

  cur_protocol.init();
}

// the loop routine runs over and over again forever:
void loop() {
  notifier.loop();

  runLoop();
}

int16_t remap(uint16_t input) {
  if (GLOBAL_CFG.midPointCorrection && input < 514 && input > 510) input = 512;
  int16_t val = map(input, 0, 1023, 1000, 2000);
  

  return val;
}

void runLoop() {
  bool changed = input.readAnalog();
  if (changed) {
    cur_protocol.setChannelValue(0, map(input.analogVals[0], 0, 1023, 1000, 2000));
    cur_protocol.setChannelValue(1, remap(input.analogVals[1] - GLOBAL_CFG.gimbalMidPointsDelta[0]));
    cur_protocol.setChannelValue(2, remap(input.analogVals[2] - GLOBAL_CFG.gimbalMidPointsDelta[1]));
    cur_protocol.setChannelValue(3, remap(input.analogVals[3] - GLOBAL_CFG.gimbalMidPointsDelta[2]));

//    // TODO: remove or configured whether print or not
//    printlog(2, ">>gas:%d,yaw:%d,roll:%d,pitch:%d", sbus.getChannelData(0), sbus.getChannelData(1), sbus.getChannelData(2), sbus.getChannelData(3));
  }

//  return; // Print analog only

  if (input.readDigital()) {
    cur_protocol.setChannelValue(4, input.currentFlightMode);
    cur_protocol.setChannelValue(6, input.aux[0]);
    cur_protocol.setChannelValue(7, input.aux[1]);
    cur_protocol.setChannelValue(8, input.aux[2]);

    changed = true;
  }

  cur_protocol.transmitAndReceive();

  // TODO: build packet if needed in protocol
//  sbus.buildPacket(dataPacket, HEADER_OFFSET);
}


