#ifndef __OUTPUT_h__
#define __OUTPUT_h__

#include "LED.h"

class Notifier {
  public:

    Notifier() : led(LED_PIN), buzzer(BUZZER_PIN) { }

    void begin() {
      // starting up
      led.blink(70, 70);
      buzzOff();
    }

    void loop() {
      led.loop();
      buzzer.loop();
    }

    // Battery is at low level
    void buzzWarnBattery() {
      priority = 1;
      buzzer.blink(500, 30);
    }

    // Battery is at very low level
    void buzzAlertBattery() {
      priority = 1;
      buzzer.blink(70, 70);
    }

    void buzzOff() {
      buzzer.blink(1000, 0);
      priority = 0;
    }

    void beep(u16 period) {
      buzzer.on(period);
    }

    void warnRf(uint8_t mode) {
      switch(mode) {
        case 0: // rssi low
          led.blink(30, 1500);
          break;
        case 1: // no telemetry received
          led.blink(60, 60);
          break;
        case 2: // Package lost
          led.blink(500, 200);
          break;
      }
      
      if (!priority)
        buzzer.blink(1500, 30);
    }

    void showOK() {
      led.blink(2000, 50);
      if (!priority)
        buzzOff();
    }

    void showAuxSelection(uint8_t auxNumber) {
      led.flash(auxNumber);
    }

  private:
    LED led;
    LED buzzer;

    u8 priority = 0;
};

#endif
