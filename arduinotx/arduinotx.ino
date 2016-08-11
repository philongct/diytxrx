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

// delaytime returned from protocol:
// the protocol expect to delay n microseconds after next call
u32 delayTime = 0;

void setup() {
  printf_begin();
  printlog(1, "Initiallizing...");
  notifier.begin();

  int inputBattery = analogRead(BATTERY_PIN);
  printlog(0, "battery %d", inputBattery);
  // expect battery > 7v (3.5*2); 511 is 5v (2.5*2)
  while(inputBattery > 511 && inputBattery < 716) {
    notifier.loop();
    notifier.buzzAlertBattery();
  }
  
  GLOBAL_CFG.load();

  if (input.calibrateGimbalMidPoint(&GLOBAL_CFG)) {
    GLOBAL_CFG.save();
  }
  
  printlog(1, "Starting now");
  GLOBAL_CFG.show(1);

  initIoPins();

  if (cur_protocol.init()) {
    while(!cur_protocol.pair()) {
      delay(1000);
    }
  }
}

// the loop routine runs over and over again forever:
void loop() {
  batteryCheck();

  runLoop();
}

void batteryCheck() {
  //battery is <6.6 (3.3*2)
  int batt = analogRead(BATTERY_PIN);
  if (batt > 511 && batt < 675) { 
    notifier.buzzAlertBattery();
  } else {
    notifier.showOK();
  }
}

int16_t remap(int16_t input) {
  if (input < 0) input = 0;             // due to midpoint
  else if (input > 1023) input = 1023;  // compenstation

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
//    printlog(2, ">>gas:%d,yaw:%d,roll:%d,pitch:%d", cur_protocol.getChannelValue(0), cur_protocol.getChannelValue(1), cur_protocol.getChannelValue(2), cur_protocol.getChannelValue(3));
  }

  if (input.readDigital()) {
    cur_protocol.setChannelValue(4, input.currentFlightMode);
    cur_protocol.setChannelValue(6, input.aux[0]);
    cur_protocol.setChannelValue(7, input.aux[1]);
    cur_protocol.setChannelValue(8, input.aux[2]);
  }

  notifier.loop();

  while (micros() < delayTime); // busy wait
  delayTime = cur_protocol.transmitAndReceive();
}


