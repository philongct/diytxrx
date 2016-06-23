#ifndef __INPUT_h__
#define __INPUT_h__

#define VR          2

#define AXIS_NUM    4
#define AUX_NUM     2

// Pin A1-A4 for yaw, gas, roll, pitch. Keep them in order
uint8_t const ANALOGS[AXIS_NUM] = {A1, A2, A3, A4};

class Input {
  public:
    uint16_t analogVals[AXIS_NUM];
    uint16_t aux[AUX_NUM];

    Input(Notifier* notifier) {
      this->notifier = notifier;
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
      
    }

    
  private:
    Notifier* notifier;
    
    uint8_t currentAux = 0;

    boolean isChange(int cur, int last) {
      int diff = last - cur;
      return (diff > VR || diff < -1 * VR);
    }
};

#endif
