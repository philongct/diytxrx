#include "SBUS.h"

void SBUS::setChannelData(uint8_t ch, int16_t value) {
  channelData[ch] = value;
}

int16_t SBUS::getChannelData(uint8_t ch) {
  return channelData[ch];
}

uint8_t* SBUS::getPacket() {
  return packetData;
}

void SBUS::buildPacket(void) {
  uint8_t packet_Position = 0;
  uint8_t current_Packet_Bit = 0;
  uint8_t current_Channel = 0;
  uint8_t current_Channel_Bit = 0;
  
  for(packet_Position = 0;packet_Position < 25; packet_Position++) packetData[packet_Position] = 0x00;  //Zero out packet data
  
  current_Packet_Bit = 0;
  packet_Position = 0;
  packetData[packet_Position] = 0x0F;  //Start Byte
  packet_Position++;
  
  for(current_Channel = 0; current_Channel < 16; current_Channel++) {
    for(current_Channel_Bit = 0; current_Channel_Bit < 11; current_Channel_Bit++) {
      if(current_Packet_Bit > 7) {
        current_Packet_Bit = 0;  //If we just set bit 7 in a previous step, reset the packet bit to 0 and
        packet_Position++;       //Move to the next packet byte
      }

      //Downshift the channel data bit, then upshift it to set the packet data byte
      packetData[packet_Position] |= (((channelData[current_Channel]>>current_Channel_Bit) & 0x01)<<current_Packet_Bit);
      current_Packet_Bit++;
    }
  }
  if(channelData[16] > 1023) packetData[23] |= (1<<0);  //Any number above 1023 will set the digital servo bit
  if(channelData[17] > 1023) packetData[23] |= (1<<1);
  if(failsafe_status == SBUS_SIGNAL_LOST) packetData[23] |= (1<<2);
  if(failsafe_status == SBUS_SIGNAL_FAILSAFE) packetData[23] |= (1<<3);
  packetData[24] = 0x00;  //End byte
}

