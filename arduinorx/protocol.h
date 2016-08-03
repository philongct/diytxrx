#ifndef __PROTOCOL_h__
#define __PROTOCOL_h__

// Macros
#define NOP() __asm__ __volatile__("nop")

enum TXRX_State {
  TXRX_OFF,
  TX_EN,
  RX_EN
};

#endif
