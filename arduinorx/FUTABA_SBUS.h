//#include <SerialPort.h> must be in the main program
//port number is changed below
#ifndef FUTABA_SBUS_h
#define FUTABA_SBUS_h

#include <Arduino.h>
#include "SoftSerial.h"


#define SBUS_SIGNAL_OK          0x00
#define SBUS_SIGNAL_LOST        0x01
#define SBUS_SIGNAL_FAILSAFE    0x03
#define BAUDRATE 99000
#define RXBUFFER 64
#define TXBUFFER 64
#define PORTNUMBER 1

class FUTABA_SBUS {
	public:
    FUTABA_SBUS(SoftSerial *invertedSerial);
    
    uint8_t failsafe_status;

    void setChannelData(uint8_t ch, int16_t value);
    int16_t getChannelData(uint8_t ch);
    
    void begin();
    void write(void);
	private:
		SoftSerial *port0;

    uint8_t packetData[25] = {
        0x0f,0x01,0x04,0x20,0x00,0xff,0x07,0x40,0x00,0x02,0x10,0x80,0x2c,0x64,0x21,0x0b,0x59,0x08,0x40,0x00,0x02,0x10,0x80,0x00,0x00};
    int16_t channelData[18] = {
        1700,1700,1700,1700,1000,1500,1300,1400,1600,1700,1800,1100,1200,1800,1900,0,0,0};

    long lastSent = millis();

    void buildPacket();
};

#endif
