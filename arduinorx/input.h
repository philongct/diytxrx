#ifndef __INPUT_h__
#define __INPUT_h__

#define FLIGH_SAFE_PIN      A0

const uint8_t CELLS[] = {A1, A2, A3};

class Input {
  public:
    void begin() {
      // Init digital pin
      pinMode(FLIGH_SAFE_PIN, INPUT);
      digitalWrite(FLIGH_SAFE_PIN, HIGH);
    }

    uint8_t getfailSafeEnabled() {
      return digitalRead(FLIGH_SAFE_PIN);
    }

    uint8_t getCellVoltage(uint8_t cellNum) {
      return analogRead(CELLS[cellNum - 1])/4;
    }
};

#endif
