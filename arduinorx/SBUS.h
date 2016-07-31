//#include <SerialPort.h> must be in the main program
//port number is changed below
#ifndef FUTABA_SBUS_h
#define FUTABA_SBUS_h

#include <Arduino.h>

#define SBUS_SIGNAL_OK          0x00
#define SBUS_SIGNAL_LOST        0x01
#define SBUS_SIGNAL_FAILSAFE    0x03

#define SBUS_BAUDRATE     99000
#define SBUS_PACKET_LEN   25

class SBUS {
  public:
    uint8_t failsafe_status;

    void setChannelData(uint8_t ch, int16_t value) {
      if (value < 0) value = 0;
      channelData[ch] = value;
    }
    
    int16_t getChannelData(uint8_t ch) {
      return channelData[ch];
    }
    
    void buildPacket(uint8_t* packetData, uint8_t offset) {
      uint8_t packet_Position = 0;
      uint8_t current_Packet_Bit = 0;
      uint8_t current_Channel = 0;
      uint8_t current_Channel_Bit = 0;
      
      for(packet_Position = 0;packet_Position < SBUS_PACKET_LEN; packet_Position++) packetData[packet_Position + offset] = 0x00;  //Zero out packet data
      
      current_Packet_Bit = 0;
      packet_Position = 0;
      packetData[packet_Position + offset] = 0x0F;  //Start Byte
      packet_Position++;
      
      for(current_Channel = 0; current_Channel < 16; current_Channel++) {
        for(current_Channel_Bit = 0; current_Channel_Bit < 11; current_Channel_Bit++) {
          if(current_Packet_Bit > 7) {
            current_Packet_Bit = 0;  //If we just set bit 7 in a previous step, reset the packet bit to 0 and
            packet_Position++;       //Move to the next packet byte
          }
    
          //Downshift the channel data bit, then upshift it to set the packet data byte
          packetData[packet_Position + offset] |= (((channelData[current_Channel]>>current_Channel_Bit) & 0x01)<<current_Packet_Bit);
          current_Packet_Bit++;
        }
      }
      if(channelData[16] > 1023) packetData[23 + offset] |= (1<<0);  //Any number above 1023 will set the digital servo bit
      if(channelData[17] > 1023) packetData[23 + offset] |= (1<<1);
      if(failsafe_status == SBUS_SIGNAL_LOST) packetData[23 + offset] |= (1<<2);
      if(failsafe_status == SBUS_SIGNAL_FAILSAFE) packetData[23 + offset] |= (1<<3);
      packetData[24 + offset] = 0x00;  //End byte
    }
    
  private:
    int16_t channelData[18] = {
        1700,1700,1700,1700,0,1500,0,0,0,0,1800,1100,1200,1800,1900,0,0,0};
};

#endif
