#ifndef __CONFIG_h__
#define __CONFIG_h__

#include <EEPROM.h>

#include "logger.h"

#define FLIGH_MODE_PIN  2

#define AUX_SEL_PIN     3
#define AUX_UP_PIN      4
#define AUX_DOWN_PIN    5

#define BUZZ_PIN        8
#define FLIGHT_PIN      6   // 6

//*******************
//***   Pinouts   ***
//*******************
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
#define SDO_1   digitalRead(SDO_pin) == 1
#define SDO_0   digitalRead(SDO_pin) == 0
//
#define SDI_1   digitalRead(SDI_pin) == 1
#define SDI_0   digitalRead(SDI_pin) == 0
//
#define SDI_SET_INPUT   pinMode(SDI_pin, INPUT)
#define SDI_SET_OUTPUT  pinMode(SDI_pin, OUTPUT)
//
#ifdef A7105_INSTALLED
#define  CS_on    digitalWrite(A7105_INSTALLED, HIGH)
#define  CS_off   digitalWrite(A7105_INSTALLED, LOW)
#endif
//
#ifdef CC2500_INSTALLED
#define CC25_CSN_on   digitalWrite(CC2500_INSTALLED, HIGH)
#define CC25_CSN_off  digitalWrite(CC2500_INSTALLED, LOW)
#endif
//

#define CONFIGID        235

void initIoPins() {
  pinMode(SDO_pin, INPUT);
  digitalWrite(SDO_pin, HIGH);
  pinMode(SDI_pin, OUTPUT);
  pinMode(SCLK_pin, OUTPUT);

#ifdef CC2500_INSTALLED
  pinMode(CC2500_INSTALLED, OUTPUT);
#endif

#ifdef A7105_INSTALLED
  pinMode(A7105_INSTALLED, OUTPUT);
#endif
}

class Config {
  public:
    char gimbalMidPointsDelta[3];
    bool midPointCorrection;
    uint8_t moduleId = 63;   // TODO: autogenerate

    Config() {
      initVariables();
    }
    
    void save() {
      EEPROM.put(0, *this);
    }

    void show(uint8_t logLevel) {
      printlog(logLevel, "Gimbal Midpoint Compenstation: %d %d %d", gimbalMidPointsDelta[0], gimbalMidPointsDelta[1], gimbalMidPointsDelta[2]);
      printlog(logLevel, "Gimbal Midpoint Correction: %d", midPointCorrection);
    }

    void load() {
      EEPROM.get(0, *this);
      // Check if this is valid config
      if (this->id != CONFIGID) {
        initVariables();
        save();
      }
    }
  private:
    uint8_t id;
  
    void initVariables() {
      id = CONFIGID;
      
      gimbalMidPointsDelta[0] = 0;
      gimbalMidPointsDelta[1] = 0;
      gimbalMidPointsDelta[2] = 0;
      
      midPointCorrection = true;
    }
} GLOBAL_CFG;

#endif
