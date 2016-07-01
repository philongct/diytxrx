#ifndef __INPUT_h__
#define __INPUT_h__

#define VR          2

#define AXIS_NUM    4
#define AUX_NUM     3

// Pin A1-A4 for yaw, gas, roll, pitch. Keep them in order
uint8_t const ANALOGS[AXIS_NUM] = {A1, A2, A3, A4};

class Input {
  public:
    uint16_t analogVals[AXIS_NUM];
    uint8_t currentFlightMode = 0;
    uint16_t aux[AUX_NUM] = {0, 0, 0};

    Input(Notifier* notifier) {
      this->notifier = notifier;

      // Init digital pin
      pinMode(AUX_SEL_PIN, INPUT);
      digitalWrite(AUX_SEL_PIN, HIGH);

      pinMode(FLIGH_MODE_PIN, INPUT);
      digitalWrite(FLIGH_MODE_PIN, HIGH);
      
      pinMode(AUX_UP_PIN, INPUT);
      digitalWrite(AUX_UP_PIN, HIGH);
      pinMode(AUX_DOWN_PIN, INPUT);
      digitalWrite(AUX_DOWN_PIN, HIGH);
    }
  
    bool readAnalog() {
      bool change = false;
      for (uint8_t i = 0; i < AXIS_NUM; ++i) {
        uint16_t analogVal = analogRead(ANALOGS[i]);
        if (isChange(analogVal, analogVals[i])) {
          analogVals[i] = analogVal;
          change = true;
        }
      }

      return change;
    }

    bool readDigital() {
      bool change = false;

      // Read AUX
      if (readPin(AUX_SEL_PIN, 500)) {
        currentAux = (currentAux + 1) % AUX_NUM;
        notifier->showAuxSelection(currentAux + 1);
      }
      
      if (readPin(AUX_UP_PIN) && aux[currentAux] < 100) {
        ++aux[currentAux];
        change = true;
      } else if (readPin(AUX_DOWN_PIN) && aux[currentAux] > 0) {
        --aux[currentAux];
        change = true;
      }

      if (readPin(FLIGH_MODE_PIN, 500)) {
        currentFlightMode = (currentFlightMode + 1) % 3;
        change = true;
      }

      return change;
    }

    
  private:
    Notifier* notifier;
    
    uint8_t currentAux = 0;

    long previousClick = 0;

    boolean isChange(int cur, int last) {
      int diff = last - cur;
      return (diff > VR || diff < -1 * VR);
    }

    // return true if pressed
    bool readPin(uint8_t pinNumber, uint8_t pressDelay = 120) {
      uint8_t state = digitalRead(pinNumber);
      if (millis() - previousClick > pressDelay && state == 0) {
        previousClick = millis();
        return true;
      }

      return false;
    }
};

#endif
