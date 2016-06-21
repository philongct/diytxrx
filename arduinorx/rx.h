#ifndef __RX_h__
#define __RX_h__

#include <RF24.h>

#define PACKET_LEN        32    // Packet length always = 30 bytes
#define PAYLOAD_LEN       25    // Max payload length
#define HEADER_OFFSET     7     // PACKET_LEN - PAYLOAD_LEN

#define SYNC_PACKET       0x01
#define DATA_PACKET       0xF0
#define SYNC_TEST_PACKET  0x0A

#define SYNC_OK           0x0F

class RX {
  public:
    RX(uint8_t cePin, uint8_t csnPin): radio(cePin, csnPin) {}
  
    void begin() {
      radio.begin();
      radio.setPALevel(RF24_PA_MAX);
      radio.setDataRate(RF24_250KBPS);
      radio.setCRCLength(RF24_CRC_8);
      radio.openWritingPipe(pipes[1]);    // note that our pipes are the same above, but that
      radio.openReadingPipe(1, pipes[0]); // they are flipped between rx and tx sides.
      radio.startListening();
#ifdef __DEBUG__
      radio.printDetails();
#endif
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
};

#endif
