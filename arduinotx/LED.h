#ifndef __LED_h__
#define __LED_h__

#include "logger.h"

class LED {
  public:
    LED(uint8_t pinNumber) {
      changePin(pinNumber);
    }

    void changePin(uint8_t pinNumber) {
      if ( pin != pinNumber) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);  // turn off
        
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
        if (state == HIGH && millis() - lastUpdate > 500) {
          state = LOW;
          lastUpdate = millis();
        } else if (state == LOW && millis() - lastUpdate > 200) {
          state = HIGH;
          lastUpdate = millis();
          --flashCount;
        }
      } else if (flashCount == 0) {
        if ( state == HIGH && millis() - lastUpdate > onPeriod * 10 ) {
          state = LOW;
          lastUpdate = millis();
          printlog(4, "LED OFF");
        } else if ( state ==  LOW && millis() - lastUpdate > offPeriod * 10) {
          state = HIGH;
          lastUpdate = millis();
          printlog(4, "LED ON");
        }
      }

      digitalWrite(pin, state);
    }

    void blink(uint8_t onInterval, uint8_t offInterval) {
      onPeriod = onInterval;
      offPeriod = offInterval;
    }

    void flash(uint8_t count) {
      if (flashCount == 0 && count > 0) {
        // turn off
        state = HIGH;
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
    uint8_t onPeriod = 0;
    uint8_t offPeriod = 0;
    uint8_t state = HIGH;
    long lastUpdate = millis();
};

#endif
