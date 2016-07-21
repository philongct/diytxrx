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

#define RF_CE_PIN       9
#define RF_CSN_PIN      10

#define CONFIGID        235

class Config {
  public:
    char gimbalMidPointsDelta[3];
    bool midPointCorrection;

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
