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

//#define DISABLE_RF

#define SBUS_MIN    193     // for cleanflight to display 1000 to
#define SBUS_MAX    1791    // 2000 and midpoint 1500

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

// workaround for input not updated at startup
u8 started = 0;

void setup() {
  printf_begin();
  printlog(1, "Initiallizing...");
  notifier.begin();

  int inputBattery = analogRead(BATTERY_PIN);
  printlog(0, "battery %d", inputBattery);
  // 511 is normal voltage (5v = 2.5*2)
  if (inputBattery > 511 && inputBattery < BATTERY_LIMIT_LO) {
    Serial.println("battery not enough");
    notifier.buzzBatteryDanger();
    while(true) {   // stop program from running
      notifier.loop();
    }
  } else if(inputBattery > 511 && inputBattery < BATTERY_LIMIT_HI) {
    Serial.println("warn battery");
    notifier.buzzWarnBattery();
    while(millis() < 3000) {  // notify for first 3s after startup
      notifier.loop();
    }
  }
  
  GLOBAL_CFG.load();

  if (input.calibrateGimbalMidPoint(&GLOBAL_CFG)) {
    GLOBAL_CFG.save();
  }
  
  printlog(1, "Starting now");
  GLOBAL_CFG.show(1);

  initIoPins();

#ifndef DISABLE_RF
  if (cur_protocol.init()) {
    while(!cur_protocol.pair()) {
      delay(1000);
    }
  }
#endif
}

// the loop routine runs over and over again forever:
void loop() {
  statusCheck();

  runLoop();
  started = 1;
}

void statusCheck() {
  //battery is <6.6 (3.3*2)
  int batt = analogRead(BATTERY_PIN);
  if (batt > 511 && batt < BATTERY_LIMIT_LO || cur_protocol.receiverStatus.battery < BATTERY_LIMIT_HI) {
    notifier.buzzWarnBattery();
  } else {
    notifier.buzzOff();
  }
  
#ifndef DISABLE_RF
  if (micros() - cur_protocol.receiverStatus.teleLastReceived > 2000000) {
    notifier.warnRf(1);
  } else if (cur_protocol.receiverStatus.packetLost > 5) {
    notifier.warnRf(2);
  } else if (cur_protocol.badSignal()) {
    notifier.warnRf(0);
  } else {
    notifier.showOK();
  }
#endif
}

int16_t remap(int16_t input) {
  if (input < 0) input = 0;             // due to midpoint
  else if (input > 1023) input = 1023;  // compenstation

  if (GLOBAL_CFG.midPointCorrection && input < 514 && input > 510) input = 512;
  int16_t val = map(input, 0, 1023, SBUS_MIN, SBUS_MAX);

  return val;
}

void runLoop() {
  bool changed = input.readAnalog();
  if (changed || !started) {
    cur_protocol.setChannelValue(0, map(input.analogVals[0], 0, 1023, SBUS_MIN, SBUS_MAX));
    cur_protocol.setChannelValue(1, remap(input.analogVals[1] - GLOBAL_CFG.gimbalMidPointsDelta[0]));
    cur_protocol.setChannelValue(2, remap(input.analogVals[2] - GLOBAL_CFG.gimbalMidPointsDelta[1]));
    cur_protocol.setChannelValue(3, remap(input.analogVals[3] - GLOBAL_CFG.gimbalMidPointsDelta[2]));

#if defined(DISABLE_RF)
    // TODO: remove or configured whether print or not
    printlog(2, ">>gas:%d,yaw:%d,roll:%d,pitch:%d", cur_protocol.getChannelValue(0), cur_protocol.getChannelValue(1), cur_protocol.getChannelValue(2), cur_protocol.getChannelValue(3));
#endif
  }

  input.readDigital();
  cur_protocol.setChannelValue(4, map(input.currentFlightMode, 0, 2, SBUS_MIN, SBUS_MAX));
  cur_protocol.setChannelValue(5, map(input.aux[0], 0, 2, SBUS_MIN, SBUS_MAX));
  cur_protocol.setChannelValue(6, map(input.aux[1], 0, 100, SBUS_MIN, SBUS_MAX));
  cur_protocol.setChannelValue(7, map(input.aux[2], 0, 100, SBUS_MIN, SBUS_MAX));
  cur_protocol.setChannelValue(8, map(input.aux[3], 0, 100, SBUS_MIN, SBUS_MAX));

  notifier.loop();

#ifndef DISABLE_RF
  while (micros() < delayTime); // busy wait
  delayTime = cur_protocol.transmitAndReceive();
#endif
}


