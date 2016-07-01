#ifndef __OUTPUT_h__
#define __OUTPUT_h__

#include "LED.h"

class Notifier {
  public:

    Notifier() : led(FLIGHT_PIN) { }

    void begin() {
//      showFlightMode();
    }

    void loop() {
      led.loop();
    }

    // Battery is at low level
    void buzzWarnBattery() {
    
    }

    // Battery is at very low level
    void buzzAlertBattery() {
    
    }

    void warnRf() {
      led.blink(10, 10);
    }

    void showOK() {
      led.blink(200, 5);
    }

    void showAuxSelection(uint8_t auxNumber) {
      led.flash(auxNumber);
    }

  private:
    LED led;

    long led_last_flash = 0;
    long buzzer_last_tone = 0;
};

#endif
