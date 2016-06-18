#ifndef __TX_h__
#define __TX_h__

#include <RF24.h>

#define TIMEOUT   250

class TX {
  public:
    TX(uint8_t cePin, uint8_t csnPin) :radio(cePin, csnPin) {}
    
    void begin() {
      radio.begin();
      radio.setPALevel(RF24_PA_LOW);
      radio.setDataRate(RF24_250KBPS);
      radio.setCRCLength(RF24_CRC_8);
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1, pipes[1]);
      radio.startListening();
      //  radio.printDetails();
    }

    void printStats() {
      Serial.print("=> -64dBm: "); Serial.println(radio.testRPD());
    }

    void transmit(void *data, uint8_t len) {
      // transmit the data
      radio.stopListening();
      radio.write( data, len );
      radio.startListening();
    }

    bool receiveUntilTimeout(void* data, uint8_t len) {
      // listen for acknowledgement from the receiver
      long started_waiting_at = millis();
      bool timeout = false;
      while (!(receive(data, len) || timeout)) {
        if ( millis() - started_waiting_at > TIMEOUT)
            timeout = true;
      }

      if (timeout){
          Serial.println("Failed, response timed out.");
      } else {
        Serial.print("Got response, round trip delay: ");
        Serial.println(millis() - started_waiting_at);
      }

      return !timeout;
    }

   bool receive(void* data, uint8_t len) {
    if (radio.available()) {
      // the receiver is just going to spit the data back
      radio.read( data, len);
      return true;
    }

    return false;
   }
  private:
    const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };
    RF24 radio;
};


#endif
