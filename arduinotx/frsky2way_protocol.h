#if defined(FRSKY_CC2500_INO) && not defined(__FRSKY2WAY_PROTOCOL_H__)
#define __FRSKY2WAY_PROTOCOL_H__

#include "protocol.h"
#include "CC2500_SPI.h"

static uint8_t fine = 0xd7;//* 215 *//give values from 0 to 255 for freq offset
                           //values from 0-127 offset increase frequency ,
                           //values from 255 to 127 decrease base frequency
                            //this is useful for tunning TX base frequency to frsky RX freq.

enum {
  FRSKY_BIND       = 0,
  FRSKY_BIND_DONE  = 1000,
  FRSKY_DATA1,
  FRSKY_DATA2,
  FRSKY_DATA3,
  FRSKY_DATA4,
  FRSKY_DATA5
};

class FrSky2WayProtocol: public Protocol {
  public:
    // will be run when device startup
    void init() {
      frsky2way_init(1);
      state = FRSKY_BIND;//
      while(state<FRSKY_BIND_DONE){// 
        unsigned long pause;    
        pause = micros();
        CC2500_Strobe(CC2500_SIDLE);
        CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
        CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
        frsky2way_build_bind_packet();
        CC2500_WriteReg(CC2500_SFRX);//0x3A
        CC2500_WriteData(packet, packet[0]+1);
        state++;
        while((micros()-pause)<9000);   
      }
if(state==FRSKY_BIND_DONE){
state=FRSKY_DATA2;
frsky2way_init(0);
counter=0;
}
    }

    // transmit & receive process
    void transmitAndReceive() {

    }
  private:
    uint32_t fixed_id = 0x3e19;
    uint8_t counter = 0;
    uint8_t state;

    void frsky2way_init(uint8_t bind){
      // Configure cc2500 for tx mode
      CC2500_Reset();
      CC2500_WriteReg(CC2500_02_IOCFG0, 0x06);    
      CC2500_WriteReg(CC2500_00_IOCFG2, 0x06);
      CC2500_WriteReg(CC2500_17_MCSM1, 0x0c);
      CC2500_WriteReg(CC2500_18_MCSM0, 0x18);
      CC2500_WriteReg(CC2500_06_PKTLEN, 0x19);//25
      CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);
      CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);
      CC2500_WriteReg(CC2500_3E_PATABLE, 0xff);
      CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x08);
      CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00);
      //base freq              FREQ = 0x5C7627 (F = 2404MHz)
      CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c); 
      CC2500_WriteReg(CC2500_0E_FREQ1, 0x76);
      CC2500_WriteReg(CC2500_0F_FREQ0, 0x27);
      //    
      CC2500_WriteReg(CC2500_10_MDMCFG4, 0xAA);   
      CC2500_WriteReg(CC2500_11_MDMCFG3, 0x39);
      CC2500_WriteReg(CC2500_12_MDMCFG2, 0x11);
      CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);
      CC2500_WriteReg(CC2500_14_MDMCFG0, 0x7a);
      CC2500_WriteReg(CC2500_15_DEVIATN, 0x42);
      CC2500_WriteReg(CC2500_19_FOCCFG, 0x16);
      CC2500_WriteReg(CC2500_1A_BSCFG, 0x6c); 
      CC2500_WriteReg(CC2500_1B_AGCCTRL2,0x03); 
      CC2500_WriteReg(CC2500_1C_AGCCTRL1,0x40);
      CC2500_WriteReg(CC2500_1D_AGCCTRL0,0x91);
      CC2500_WriteReg(CC2500_21_FREND1, 0x56);
      CC2500_WriteReg(CC2500_22_FREND0, 0x10);
      CC2500_WriteReg(CC2500_23_FSCAL3, 0xa9);
      CC2500_WriteReg(CC2500_24_FSCAL2, 0x0A);
      CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);
      CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);
      CC2500_WriteReg(CC2500_29_FSTEST, 0x59);
      CC2500_WriteReg(CC2500_2C_TEST2, 0x88);
      CC2500_WriteReg(CC2500_2D_TEST1, 0x31);
      CC2500_WriteReg(CC2500_2E_TEST0, 0x0B);
      CC2500_WriteReg(CC2500_03_FIFOTHR, 0x07);
      CC2500_WriteReg(CC2500_09_ADDR, 0x00);

      CC2500_SetTxRxMode(TX_EN);  // set GDO0/GDO2 depend on tx or rx mode
      CC2500_SetPower(bind ? CC2500_BIND_POWER : CC2500_HIGH_POWER);
      
      CC2500_Strobe(CC2500_SIDLE);    
      
      CC2500_WriteReg(CC2500_02_IOCFG0, 0x06);
      CC2500_WriteReg(CC2500_09_ADDR, bind ? 0x03 : (fixed_id & 0xff));
      CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);      
      CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  }
};


#endif
