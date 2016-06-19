#ifndef __TX_h__
#define __TX_h__

#include <RF24.h>

#define PACKET_LEN        32    // Packet length always = 30 bytes
#define PAYLOAD_LEN       25    // Max payload length
#define HEADER_OFFSET     7     // PACKET_LEN - PAYLOAD_LEN

#define SYNC_PACKET       0x01
#define DATA_PACKET       0xF0
#define SYNC_TEST_PACKET  0x0A

#define SYNC_OK           0x0F

#define TIMEOUT       250   // receive timeout = 250 milliseconds

class TX {
  public:
    TX(uint8_t cePin, uint8_t csnPin) :radio(cePin, csnPin) {}
    
    void begin() {
      radio.begin();
      radio.setPALevel(RF24_PA_LOW);
      radio.setDataRate(RF24_250KBPS);
      radio.setCRCLength(RF24_CRC_8);
//      radio.setChannel(58);
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1, pipes[1]);
      radio.startListening();
      //  radio.printDetails();
      sync();
    }

    void sync() {
      uint8_t packet[PACKET_LEN];
      buildSyncPacket(packet);
      transmitPacket(packet, true);
      
      if (receiveUntilTimeout(&packet)) {
        if (packet[0] == SYNC_OK) {
          Serial.print("SYNC OK, channel is: ");Serial.println(packet[1]);
          // TODO: Set new channel here
          buildSyncTestPacket(packet);
          transmitPacket(packet, true);
          
          if (receiveUntilTimeout(&packet)) {
            if (packet[0] == SYNC_OK) {
              Serial.println("SYNC TEST OK");
              synced = true;
            }
          }
        }
      }
    }

    void testSignal() {
      uint8_t numCh = 100;
      uint8_t signals[numCh];
      uint8_t ch = radio.getChannel();
      for (uint8_t i = 0; i < numCh; ++i) {
        signals[i] = 0;
        // Select this channel
        radio.stopListening();
        radio.setChannel(i);
        // Listen for a little
        radio.startListening();
        for (uint8_t j = 0; j < 100; ++j) {
          delayMicroseconds(225);
          // Did we get a carrier?
          if ( radio.testRPD() ) {
            signals[i] += 1;
          }
        }
      }

      uint8_t bestCh[2] = { ch, 0 };
      for (uint8_t i = 0; i < numCh; ++i) {
        if (signals[i] > bestCh[1]) {
          bestCh[0] = i;
          bestCh[1] = signals[i];
        }
      }
      Serial.println("Best channel " + String(bestCh[0]) + ": " + String(bestCh[1]));

      radio.setChannel(ch);
      radio.startListening();
    }

    void buildDataPacket(uint8_t* buff) {
      buff[0] = DATA_PACKET;
    }

    bool transmitPacket(void *packet, boolean force=false) {
      if (force || synced) {
        Serial.println("Sending...");
        // transmit the data
        radio.stopListening();
        radio.write( packet, PACKET_LEN );
        radio.startListening();
        return true;
      }

      return false;
    }

    bool receiveUntilTimeout(void* buff, uint8_t timeOutMs = TIMEOUT) {
      // listen for acknowledgement from the receiver
      long started_waiting_at = millis();
      bool timeout = false;
      while (!(receive(buff) || timeout)) {
        if ( millis() - started_waiting_at > timeOutMs)
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

   bool receive(void* buff) {
    if (radio.available()) {
      boolean done = false;
      while (!done) {
        done = radio.read( buff, PACKET_LEN);
      }
      return true;
    }

    return false;
   }

  private:
    const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };
    RF24 radio;

    bool synced = false;

    void buildSyncPacket(uint8_t* buff) {
      buff[0] = SYNC_PACKET;
    }

    void buildSyncTestPacket(uint8_t* buff) {
      buff[0] = SYNC_TEST_PACKET;
    }
};


#endif
