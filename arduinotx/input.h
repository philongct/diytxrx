#ifndef __INPUT_h__
#define __INPUT_h__

#define VR                  2

#define AXIS_NUM            4
#define AUX_NUM             3
#define FLIGHT_MODE_NUM     3

// Pin A1-A4 for yaw, gas, roll, pitch. Keep them in order
uint8_t const ANALOGS[AXIS_NUM] = {A0, A1, A2, A3};
// FlightMode selection, AUX selection, AUX up, AUX down
u8 const DIGITALS[] = {2, 3, 4, 5};

class Input {
  public:
    int16_t analogVals[AXIS_NUM];
    uint8_t currentFlightMode = 0;
    // Index 0 is current AUX selection, index 1 is up/down indicator
    // AUX values go from 1 - 100
    uint8_t aux[AUX_NUM + 2] = {0, 50, 50, 50, 50};

    Input(Notifier* notifier) {
      this->notifier = notifier;

      // Init digital pin
      for (u8 i = 0; i < sizeof(DIGITALS); ++i) {
        pinMode(DIGITALS[i], INPUT);
        digitalWrite(DIGITALS[i], HIGH);
      }
    }

    bool calibrateGimbalMidPoint(Config *GLOBAL_CFG) {
      readAnalog();
      // Check if all controls are in middle position.
      // If they are then perform calibration
      uint8_t calibrate = 0;
      for (uint8_t i = 0; i < AXIS_NUM; ++i) {
        // 40% - 60% estimation
        if (analogVals[i] > 408 && analogVals[i] < 613)
          ++calibrate;
      }

      if (calibrate == AXIS_NUM) {
        readAnalog();
        printlog(1, "Calibrating gimbal midpoint...");
        // Do calibration. Ignore throttle (analogVals[0])
        for (uint8_t i = 1; i < AXIS_NUM; ++i) {
          GLOBAL_CFG->gimbalMidPointsDelta[i - 1] = analogVals[i] - 512;
          printlog(1, "%d: %d diff %d", i, analogVals[i], GLOBAL_CFG->gimbalMidPointsDelta[i - 1]);
        }
        printlog(1, "Done");
      }

      return calibrate == AXIS_NUM;
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

    void readDigital() {
      // Read AUX
      if (readPin(DIGITALS[1]) == 2) { // AUX selection
        // init value is 0, so increase by 1 and modulo until
        // value reaching max value and turns back to 0
        aux[0] = (aux[0] + 1) % AUX_NUM;
        notifier->showAuxSelection(aux[0] + 1);
      }

      u8 up = readPin(DIGITALS[2]);
      u8 down = readPin(DIGITALS[3]);
      if (up) {
        if (up == 2 && aux[aux[0] + 2] < 100) {  // check aux up
          ++aux[aux[0] + 2];
        }
        aux[1] = 100;
      } else if (down) {
        if (down == 2 && aux[aux[0] + 2] > 0) { // check aux down
          --aux[aux[0] + 2];
        }
        aux[1] = 0;
      } else {
        aux[1] = 50;
      }

      if (readPin(DIGITALS[0]) == 2) {   // read flight mode
        currentFlightMode = (currentFlightMode + 1) % FLIGHT_MODE_NUM;
      }
    }

    
  private:
    Notifier* notifier;

    // button state:
    int8_t activeButton = -1;  // -1 mean no button pressed otherwise button number
    u32 buttonPressTimeStamp = 0xFFFFFFFF;

    boolean isChange(int cur, int last) {
      int diff = last - cur;
      return (diff > VR || diff < -1 * VR);
    }

    // return true if pressed
    u8 readPin(uint8_t pinNumber, uint8_t pressDelay = 120) {
      uint8_t state = digitalRead(pinNumber);
      if (state == LOW) {
        if (activeButton == -1) {  // if no button pressed
          activeButton = pinNumber; // mark a button pressed
          buttonPressTimeStamp = millis();
        } else if (activeButton == pinNumber && millis() -  buttonPressTimeStamp > pressDelay) {
          buttonPressTimeStamp = millis();
          return 2;
        }
      } else if (state == HIGH && pinNumber == activeButton && millis() -  buttonPressTimeStamp > pressDelay) { // this is to fix button jittering
        activeButton = -1; // mark button previously pressed released
        buttonPressTimeStamp = 0xFFFFFFFF;
      }

      return 0 || (activeButton == pinNumber) && !state;
    }
};

#endif
