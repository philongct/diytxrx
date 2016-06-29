#ifndef __TX_h__
#define __TX_h__

#include <RF24.h>

#define AVAILABLE_CH      124
#define DEFAULT_CH        104

#define PACKET_LEN        32    // Packet length always = 30 bytes
#define PAYLOAD_LEN       25    // Max payload length
#define HEADER_OFFSET     7     // PACKET_LEN - PAYLOAD_LEN

#define SYNC_PACKET       0x01
#define SYNC_TEST_PACKET  0x0A
#define SYNC_OK           0x0F

#define STAMP             0xEC
#define DATA_PACKET       0xF0
#define PROBE_PACKET      0xFF

#define TIMEOUT       250   // receive timeout = 250 milliseconds

class TX {
  public:
    TX(uint8_t cePin, uint8_t csnPin) :radio(cePin, csnPin) {}
    
    void begin() {
      init(DEFAULT_CH);
    }

    void doTestSignal(uint8_t* signalsBuffer) {
      uint8_t currentCh = radio.getChannel();
      uint8_t bestCh[2] = {currentCh, 0};

      memset(signalsBuffer, 0, AVAILABLE_CH);
      
      // Scan all channels num_reps times
      uint8_t num_reps = 100;
      while (num_reps--) {
        uint8_t num_channel = AVAILABLE_CH;
        while (num_channel--) {
          // Select this channel
          radio.stopListening();
          radio.setChannel(num_channel);
    
          // Listen for a little
          radio.startListening();
          delayMicroseconds(128);
    
          // Did we get a carrier?
          if ( radio.testRPD() )
            ++signalsBuffer[num_channel];
        }
      }
      // revert back
      radio.setChannel(currentCh);
    }

    bool sync() {
      uint8_t packet[PACKET_LEN];

      if (!scanTarget()) return synced;

      // Radio noise when switching to better channel
      synced = true;
      return true;

      buildSyncPacket(packet);
      transmitPacket(packet, true);

      uint8_t newCh = 255;
      if (receiveUntilTimeout(&packet, 15000)) {
        if (packet[0] == SYNC_OK) {
          newCh = packet[1];
          Serial.print("SYNC OK, channel is: ");Serial.println(newCh);
        }
      }

      if (newCh < 255) {
        // Sync test response on old channel
        buildSyncTestPacket(packet);
        transmitPacket(packet, true);

        // Set new channel. TODO: do sync test again?
        Serial.println("set new channel");
        setWorkingCh(newCh);
        synced = true;
      }

      return synced;
    }

    void buildDataPacket(uint8_t* buff) {
      buff[0] = DATA_PACKET;
      stampHeader(buff);
    }

    bool transmitPacket(void *packet, boolean force=false) {
      if (force || synced) {
        Serial.print("Sending on channel ");Serial.println(radio.getChannel());
        // transmit the data
        radio.stopListening();
        radio.write( packet, PACKET_LEN );
        radio.startListening();
        return true;
      }

      return false;
    }

    bool receiveUntilTimeout(void* buff, uint16_t timeOutMs = TIMEOUT) {
      // listen for acknowledgement from the receiver
      long started_waiting_at = millis();
      bool timeout = false;
      while (!(receive(buff) || timeout)) {
        uint16_t diff = millis() - started_waiting_at;
        if ( diff > timeOutMs) {
            timeout = true;
        }
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

    void init(uint8_t ch) {
      radio.begin();
      radio.setPALevel(RF24_PA_MAX);
      radio.setDataRate(RF24_250KBPS);
      radio.setCRCLength(RF24_CRC_8);
      radio.setAutoAck(false);
      radio.setChannel(ch);
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1, pipes[1]);
      radio.startListening();
//      radio.printDetails();
    }

    bool scanTarget() {
      uint8_t packetBuffer[PACKET_LEN];

      uint8_t range = AVAILABLE_CH + DEFAULT_CH;
      for (uint8_t i = DEFAULT_CH; i < range; ++i) {
        setWorkingCh(i%AVAILABLE_CH);
        buildProbePacket(packetBuffer);
        transmitPacket(packetBuffer, true);
        if (receiveUntilTimeout(&packetBuffer)) {
          if (packetBuffer[0] == PROBE_PACKET) {
            return true;
          }
        }
      }

      return false;
    }

    void setWorkingCh(uint8_t ch) {
      if (ch != radio.getChannel()) {
        radio.stopListening();
        radio.setChannel(ch);
        radio.startListening();
      }
    }

    void buildSyncPacket(uint8_t* buff) {
      buff[0] = SYNC_PACKET;
      stampHeader(buff);
    }

    void buildSyncTestPacket(uint8_t* buff) {
      buff[0] = SYNC_TEST_PACKET;
      stampHeader(buff);
    }

    void buildProbePacket(uint8_t* buff) {
      buff[0] = PROBE_PACKET;
      stampHeader(buff);
    }

    void stampHeader(uint8_t* buff) {
      buff[1] = STAMP;
    }
};


#endif
