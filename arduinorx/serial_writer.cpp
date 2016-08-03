
// When set, _DEBUG co-opts pins 11 and 13 for debugging with an
// oscilloscope or logic analyzer.  Beware: it also slightly modifies
// the bit times, so don't rely on it too much at high baud rates
#define _DEBUG 0
#define _DEBUG_PIN1 11
#define _DEBUG_PIN2 13
// 
// Includes
// 
#include <Arduino.h>
#include <util/delay_basic.h>
#include <util/parity.h>

#include "serial_writer.h"

//
// Debugging
//
// This function generates a brief pulse
// for debugging or measuring on an oscilloscope.
#if _DEBUG
inline void DebugPulse(uint8_t pin, uint8_t count)
{
  volatile uint8_t *pport = portOutputRegister(digitalPinToPort(pin));

  uint8_t val = *pport;
  while (count--)
  {
    *pport = val | digitalPinToBitMask(pin);
    *pport = val;
  }
}
#else
inline void DebugPulse(uint8_t, uint8_t) {}
#endif

//
// Private methods
//

/* static */ 
inline void SerialWriter::tunedDelay(uint16_t delay) {
  _delay_loop_2(delay);
}

//
// Constructor
//
SerialWriter::SerialWriter(uint8_t transmitPin, bool inverse_logic /* = false */) :
  _tx_delay(0),
  _inverse_logic(inverse_logic)
{
  setTX(transmitPin);
}

void SerialWriter::setTX(uint8_t tx)
{
  // First write, then set output. If we do this the other way around,
  // the pin would be output low for a short while before switching to
  // output high. Now, it is input with pullup for a short while, which
  // is fine. With inverse logic, either order is fine.
  digitalWrite(tx, _inverse_logic ? LOW : HIGH);
  pinMode(tx, OUTPUT);
  _transmitBitMask = digitalPinToBitMask(tx);
  uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

uint16_t SerialWriter::subtract_cap(uint16_t num, uint16_t sub) {
  if (num > sub)
    return num - sub;
  else
    return 1;
}

//
// Public methods
//

void SerialWriter::begin(long speed)
{
  // Precalculate the various delays, in number of 4-cycle delays
  uint16_t bit_delay = (F_CPU / speed) / 4;

  // 12 (gcc 4.8.2) or 13 (gcc 4.3.2) cycles from start bit to first bit,
  // 15 (gcc 4.8.2) or 16 (gcc 4.3.2) cycles between bits,
  // 12 (gcc 4.8.2) or 14 (gcc 4.3.2) cycles from last bit to stop bit
  // These are all close enough to just use 15 cycles, since the inter-bit
  // timings are the most critical (deviations stack 8 times)
  _tx_delay = subtract_cap(bit_delay, 15 / 4);

  tunedDelay(_tx_delay); // if we were low this establishes the end

#if _DEBUG
  pinMode(_DEBUG_PIN1, OUTPUT);
  pinMode(_DEBUG_PIN2, OUTPUT);
#endif
}


size_t SerialWriter::write(uint8_t byte)
{
  write(byte, false, false);
}

size_t SerialWriter::write(uint8_t b, bool evenParity, bool stopbit2)
{
  if (_tx_delay == 0) {
    return 0;
  }

  // By declaring these as local variables, the compiler will put them
  // in registers _before_ disabling interrupts and entering the
  // critical timing sections below, which makes it a lot easier to
  // verify the cycle timings
  volatile uint8_t *reg = _transmitPortRegister;
  uint8_t reg_mask = _transmitBitMask;
  uint8_t inv_mask = ~_transmitBitMask;
  uint8_t oldSREG = SREG;
  bool inv = _inverse_logic;
  uint16_t delay = _tx_delay;
  
  uint8_t parity = parity_even_bit(b);

  if (inv) {
    b = ~b;
	  parity = ~parity;
  }

  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  if (inv)
    *reg |= reg_mask;
  else
    *reg &= inv_mask;

  tunedDelay(delay);

  // Write each of the 8 bits
  for (uint8_t i = 8; i > 0; --i)
  {
    if (b & 1) // choose bit
      *reg |= reg_mask; // send 1
    else
      *reg &= inv_mask; // send 0
	
    tunedDelay(delay);
    b >>= 1;
  }
  
  // write parity if needed
  if ( evenParity ) {
    if (parity & 1) // choose bit
      *reg |= reg_mask; // send 1
    else
      *reg &= inv_mask; // send 0

    tunedDelay(delay);
  }

  // restore pin to natural state
  if (inv)
    *reg &= inv_mask;
  else
    *reg |= reg_mask;

  SREG = oldSREG; // turn interrupts back on
  if ( stopbit2 ) {
    tunedDelay(_tx_delay << 1);
  } else {
    tunedDelay(_tx_delay);
  }
  
  return 1;
}

