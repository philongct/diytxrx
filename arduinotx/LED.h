#ifndef __LED_h__
#define __LED_h__

class LED {
  public:
    LED(uint8_t pinNumber) {
      changePin(pinNumber);
    }

    void changePin(uint8_t pinNumber) {
      pin = pinNumber;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, state);
    }

    void loop() {
      if ( state == HIGH && millis() - lastUpdate > onPeriod * 10 ) {
        state = LOW;
        lastUpdate = millis();
//        Serial.println("LED OFF");
      } else if ( state ==  LOW && millis() - lastUpdate > offPeriod * 10) {
        state = HIGH;
        lastUpdate = millis();
//        Serial.println("LED ON");
      }

      digitalWrite(pin, state);
    }

    void flash(uint8_t onInterval, uint8_t offInterval) {
      onPeriod = onInterval;
      offPeriod = offInterval;
    }
    
  private:
    uint8_t pin;
    uint8_t onPeriod = 0;
    uint8_t offPeriod = 0;
    uint8_t state = HIGH;
    long lastUpdate = millis();
};

#endif
