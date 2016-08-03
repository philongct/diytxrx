
#ifndef __SERIAL_WRITER_H__
#define __SERIAL_WRITER_H__

class SerialWriter
{
private:
  uint8_t _transmitBitMask;
  volatile uint8_t *_transmitPortRegister;
  uint16_t _tx_delay;

  uint16_t _inverse_logic:1;

  void setTX(uint8_t transmitPin);

  // Return num - sub, or 1 if the result would be < 1
  static uint16_t subtract_cap(uint16_t num, uint16_t sub);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);

public:
  // public methods
  SerialWriter(uint8_t transmitPin, bool inverse_logic = false);
  void begin(long speed);

  virtual size_t write(uint8_t byte);
  size_t write(uint8_t byte, bool evenParity=false, bool stopbit2=false);
  operator bool() { return true; }
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif

