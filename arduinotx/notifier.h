#ifndef __OUTPUT_h__
#define __OUTPUT_h__

#include "LED.h"

class Notifier {
  public:

    Notifier() : led(FLIGHT_PIN) { }

    void begin() {
      showFlightMode();
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
      led.flash(10, 10);
    }

    void showFlightMode() {
      led.flash(250, 0);
    }

    void showAuxSelection() {
      
    }

  private:
    LED led;

    long led_last_flash = 0;
    long buzzer_last_tone = 0;
};

#endif
