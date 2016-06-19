#ifndef __OUTPUT_h__
#define __OUTPUT_h__

#define BUZZ_PIN        8
#define RF_STATUS_PIN   7
#define FLIGHT_PIN      6

class Output {
  public:

    void begin() {
      
    }

    // Battery is at low level
    void buzzWarnBattery() {
    
    }

    // Battery is at very low level
    void buzzAlertBattery() {
    
    }

    void warnWeakRf() {
      
    }

    void showFlightMode() {
      
    }

    void showAuxSelection() {
      
    }

  private:
    uint8_t led_state = 0;  // 0: showing flight mode; 1: showing radio rf
    uint8_t buzzState = 0;  // 0: showing aux selection; 1: showing flight mode; 2: warn battery; 3: alert battery

    long led_last_flash = 0;
    long buzzer_last_tone = 0;
};

#endif
