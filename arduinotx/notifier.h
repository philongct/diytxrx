#ifndef __OUTPUT_h__
#define __OUTPUT_h__

#include "LED.h"

class Notifier {
  public:

    Notifier() : led(LED_GREEN_PIN) { }

    void begin() {
      // starting up
      led.blink(7, 7);
    }

    void loop() {
      led.loop();
    }

    // Battery is at low level
    void buzzWarnBattery() {
      priority = 1;
      led.changePin(LED_RED_PIN);
      led.blink(15, 15);
    }

    // Battery is at very low level
    void buzzAlertBattery() {
      priority = 1;
      led.changePin(LED_RED_PIN);
      led.blink(6, 6);
    }

    void warnRf(uint8_t mode) {
      if (priority != 1) {
        led.changePin(LED_GREEN_PIN);
        if (mode == 0)         // rssi low
          led.blink(10, 10);
        else if (mode == 1)    // Package lost
          led.blink(6, 6);
      }
    }

    void showOK() {
      priority = 0;
      led.changePin(LED_GREEN_PIN);
      led.blink(200, 5);
    }

    void showAuxSelection(uint8_t auxNumber) {
      led.flash(auxNumber);
    }

  private:
    LED led;

    u8 priority = 0;

    long led_last_flash = 0;
    long buzzer_last_tone = 0;
};

#endif
