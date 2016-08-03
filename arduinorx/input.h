#ifndef __INPUT_h__
#define __INPUT_h__

//*******************
//***   Pinouts   ***
//*******************
#define RF_CSN      8
#define SDI_pin     11           //SDI/MOSI-D11
#define SDO_pin     12           //SDO/MISO-D12
#define SCLK_pin    13           //SCK-D13
// MACROS
#define  SCK_on   digitalWrite(SCLK_pin, HIGH)
#define  SCK_off  digitalWrite(SCLK_pin, LOW)
//
#define  SDI_on   digitalWrite(SDI_pin, HIGH)
#define  SDI_off  digitalWrite(SDI_pin, LOW)
//
#define CC25_CSN_on   digitalWrite(RF_CSN, HIGH)
#define CC25_CSN_off  digitalWrite(RF_CSN, LOW)
//
#define SDO_1   digitalRead(SDO_pin) == 1
#define SDO_0   digitalRead(SDO_pin) == 0
//
#define SDI_1   digitalRead(SDI_pin) == 1
#define SDI_0   digitalRead(SDI_pin) == 0
////

#define FLIGH_SAFE_PIN      A2

const uint8_t CELLS[] = {A0, A1};

class Input {
  public:
    void begin() {
      pinMode(SDO_pin, INPUT);
      pinMode(SDI_pin, OUTPUT);
      pinMode(SCLK_pin, OUTPUT);
      pinMode(RF_CSN, OUTPUT);
      digitalWrite(RF_CSN, HIGH);
      
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
