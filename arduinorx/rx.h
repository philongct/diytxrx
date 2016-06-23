#ifndef __RX_h__
#define __RX_h__

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
#define PROBE_PACKET      0xFF
#define DATA_PACKET       0xF0

class RX {
  public:
    RX(uint8_t cePin, uint8_t csnPin): radio(cePin, csnPin) {}
  
    void begin() {
      init(DEFAULT_CH);
    }

    uint8_t getCurrentCh() {
      return radio.getChannel();
    }

    void setWorkingCh(uint8_t ch) {
      if (ch != radio.getChannel()) {
        radio.stopListening();
        radio.setChannel(ch);
        radio.startListening();
      }
    }

    uint8_t testSignal() {
      uint8_t buffer1[AVAILABLE_CH];
      uint8_t buffer2[AVAILABLE_CH];
      doTestSignal(buffer1);
      doTestSignal(buffer2);

      uint8_t bestVal = 0;
      uint8_t goodCh = 255;       // not found yet
      uint8_t candidateCh = 255;  // not found yet
      for (uint8_t i = AVAILABLE_CH - 1; i < 255; --i) {
        if (buffer1[i] > bestVal && buffer2[i] > bestVal) {
          bestVal = min(buffer1[i], buffer2[i]);
          candidateCh = i;
          if (buffer1[i] == buffer2[i]) goodCh = i;
        }
      }

      Serial.print("Selected channels ");Serial.print(goodCh);Serial.print(", ");Serial.print(candidateCh); Serial.print(". "); Serial.println(bestVal);
      return goodCh != 255 ? goodCh : candidateCh;
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

//          Serial.print(min(0xf,signalsBuffer[num_channel]&0xf), HEX);
        }
      }
      // revert back
      Serial.println("");
      radio.setChannel(currentCh);
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
    
    void transmitPacket(void *packet) {
      // transmit the data
      radio.stopListening();
      radio.write( packet, PACKET_LEN );
      radio.startListening();
    }

    void syncResponse(uint8_t channel) {
      uint8_t packet[PACKET_LEN];
      packet[0] = SYNC_OK;
      packet[1] = channel;
      transmitPacket(&packet);
    }

    void syncTestResponse() {
      uint8_t packet[PACKET_LEN];
      packet[0] = SYNC_OK;
      transmitPacket(&packet);
    }
    
  private:
    const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };
#ifdef __DEBUG__
    RF24Debug radio;
#else
    RF24 radio;
#endif

    void init(uint8_t ch) {
      radio.begin();
      radio.setPALevel(RF24_PA_MAX);
      radio.setDataRate(RF24_250KBPS);
      radio.setCRCLength(RF24_CRC_8);
      radio.setAutoAck(false);
      radio.setChannel(ch);
      radio.openWritingPipe(pipes[1]);    // note that our pipes are the same above, but that
      radio.openReadingPipe(1, pipes[0]); // they are flipped between rx and tx sides.
      radio.startListening();
#ifdef __DEBUG__
      radio.printDetails();
#endif
    }
};

#endif
