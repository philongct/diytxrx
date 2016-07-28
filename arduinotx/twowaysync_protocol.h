#ifndef __TWO_WAY_SYNC_PROTOCOL_H__
#define __TWO_WAY_SYNC_PROTOCOL_H__

#include "protocol.h"

#define PKT_HEAD          235

#define HELLO_PKT         255
#define WELCOMEBACK_PKT   253

typedef struct HelloPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = HELLO_PKT;
  uint8_t addr;
} HelloPkt;

typedef struct WelcomebackPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = WELCOMEBACK_PKT;
  uint8_t addr;
} HelloResPkt;

class TwoWaySyncProtocol: public Protocol {
  public:
    // will be run when device startup
    void init() {
      
    }

    // when tx & rx are new from each other
    void bind() {
      
    }

    // transmit & receive process
    void transmitAndReceive() {
      
    }
}

#endif
