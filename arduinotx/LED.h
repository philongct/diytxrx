#ifndef __LED_h__
#define __LED_h__

#include "logger.h"

#define ON    LOW
#define OFF   HIGH

class LED {
  public:
    LED(uint8_t pinNumber) {
      changePin(pinNumber);
    }

    void changePin(uint8_t pinNumber) {
      if ( pin != pinNumber) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, OFF);  // turn off
        
        pin = pinNumber;
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state);
      }
    }

    void loop() {
      // Delay 1s after last flash
      if (flashCount == 1 && millis() - lastUpdate > 1000) {
        flashCount = 0;
      } else if ( flashCount > 1) {
        if (state == OFF && millis() - lastUpdate > 500) {
          state = ON;
          lastUpdate = millis();
        } else if (state == ON && millis() - lastUpdate > 200) {
          state = OFF;
          lastUpdate = millis();
          --flashCount;
        }
      } else if (flashCount == 0) {
        if ( onPeriod == 0 || (state == ON && millis() - lastUpdate > offPeriod)) {
          state = OFF;
          lastUpdate = millis();
          printlog(4, "LED OFF");
        } else if ( state == OFF && millis() - lastUpdate > onPeriod ) {
          state = ON;
          lastUpdate = millis();
          printlog(4, "LED ON");
        }
      }

      digitalWrite(pin, state);
    }

    void blink(uint16_t offInterval, uint16_t onInterval) {
      onPeriod = offInterval;
      offPeriod = onInterval;
    }

    void on(u16 period) {
      digitalWrite(pin, ON);
      delay(period);
      digitalWrite(pin, state);
    }

    void flash(uint8_t count) {
      if (flashCount == 0 && count > 0) {
        // turn off
        state = OFF;
        digitalWrite(pin, state);
        // Turn off 500ms first
        lastUpdate = millis();
        // Delay a period before last flash
        flashCount = count + 1;
      }
    }
    
  private:
    uint8_t flashCount = 0;
  
    uint8_t pin;
    uint16_t onPeriod = 0;
    uint16_t offPeriod = 0;
    uint8_t state = HIGH;   // off by default
    long lastUpdate = millis();
};

#endif
