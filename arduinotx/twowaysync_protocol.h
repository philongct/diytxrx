#if defined(TWOWAYSYNC_CC2500_INO) && not defined(__TWO_WAY_SYNC_PROTOCOL_H__)
#define __TWO_WAY_SYNC_PROTOCOL_H__

#include "protocol.h"
#include "CC2500_SPI.h"
#include "crc.h"

#define DEFAULT_ID        0x01

#define BIND_CHANNEL      0x4A

#define HOP_CH            15    // Number of channels for hopping

#define FIXED_PKT_LEN     25    // fixed radio packet length
#define MAX_PKT           27    // radio config length(25) + rssi(1) + crc status(1) = 27

#define PKT_HEAD          235

#define HELLO_PKT         255
#define WELCOMEBACK_PKT   253
#define TEST_PKT          252
#define PAIR_PKT          251
#define DATA_PKT          11
#define TELE_PKT          12

#define CC2500_MAX_POWER    CC2500_POWER_17
#define CC2500_HIGH_POWER   CC2500_POWER_16
#define CC2500_LOW_POWER    CC2500_POWER_13
#define CC2500_RANGE_POWER  CC2500_POWER_1
#define CC2500_BIND_POWER   CC2500_POWER_5

typedef struct HelloPkt {
  uint8_t len = sizeof(HelloPkt);
  uint8_t addr;
  uint8_t pkt_type = HELLO_PKT;
  uint8_t padding[FIXED_PKT_LEN - 3];  // remaining bytes to fixed packet length
} HelloPkt;

typedef struct WelcomebackPkt {
  uint8_t len = sizeof(WelcomebackPkt);
  uint8_t addr;
  uint8_t pkt_type = WELCOMEBACK_PKT;
  uint8_t paired_channels[HOP_CH];
  uint8_t padding[FIXED_PKT_LEN - HOP_CH - 3]; // remaining bytes to fixed packet length
} WelcomebackPkt;

typedef struct PairStartPkt {
  uint8_t len = sizeof(PairStartPkt);
  uint8_t addr;
  uint8_t pkt_type = PAIR_PKT;
  uint8_t padding[FIXED_PKT_LEN - 3]; // remaining bytes to fixed packet length
} PairStartPkt;

typedef struct SyncTestPkt {
  uint8_t len = sizeof(SyncTestPkt);
  uint8_t addr;
  uint8_t pkt_type = TEST_PKT;
  uint8_t rssi = 0;
  uint8_t padding[FIXED_PKT_LEN - 4]; // remaining bytes to fixed packet length
} SyncTestPkt;

typedef struct DataPkt {
  uint8_t len = sizeof(SyncTestPkt);
  uint8_t addr;
  uint8_t pkt_type = DATA_PKT;
  uint8_t data [16];
  uint8_t padding[FIXED_PKT_LEN - 16 - 3]; // remaining bytes to fixed packet length
} DataPkt;

typedef struct ReceiverStatusPkt {
  uint8_t len = sizeof(ReceiverStatusPkt);
  uint8_t addr;
  uint8_t pkt_type = TELE_PKT;
  uint8_t packetLost = 0;
  uint8_t lqi = 0;
  int8_t rssi = 0;
  uint16_t battery = 800; // min battery
  // end params transmitted from receiver
  uint16_t cycleCount = 0;
  uint16_t error_pkts = 0;        // count number of error packets
  u32 teleLastReceived;           // last timestamp telemetry packet was received at tx
  uint8_t padding[FIXED_PKT_LEN - 16]; // remaining bytes to fit FIXED_PKT_LEN
} ReceiverStatusPkt;

const PROGMEM uint8_t hop_data[] = {
  0x05, 0xD5, 0xBC, 0xA3, 0x8A,
  0x71, 0x58, 0x3F, 0x26, 0x0D,
  0xDF, 0xC6, 0xAD, 0x94, 0x7B,
  0x62, 0x49, 0x30, 0x17, 0xE7,
  0xD0, 0xB7, 0x9E, 0x85, 0x6C,
  0x53, 0x3A, 0x21, 0x08, 0xDA,
  0xC1, 0xA8, 0x8F, 0x76, 0x5D,
  0x44, 0x2B, 0x12, 0xE4, 0xCB,
  0xB2, 0x99, 0x80, 0x67, 0x4E,
  0x35, 0x1C, 0x01, 0x1E, 0x04
};

// interval between transmits. Inspired by SBUS interval
const u32 TRANSMISSION_INTERVAL = 14500;  // Actual value measured is ~14508us

class TwoWaySyncProtocol: public Protocol {
  public:
    ReceiverStatusPkt receiverStatus;   // stored receiver status (via telemetry)

    // will be run when device startup
    bool init() {
      CC2500_Reset();
      delay(500); // wait for module successfully reset

      HelloPkt hello;

      resetSettings(0);
      // packetLost must turn to 0 after sucessfully paired
      receiverStatus.packetLost = 100;

      do {
        delay(1000);  // wait a while
        Serial.println("sending hello...");
        transmit(BIND_CHANNEL, (u8*)&hello);
      } while (receive(packet_buff, 100000) == false); // wait until

      WelcomebackPkt* res = (WelcomebackPkt*)&packet_buff[0];
      if (res->pkt_type == WELCOMEBACK_PKT && res->addr == fixed_id) {
        memcpy(hop_channels, res->paired_channels, HOP_CH);
        curChannel = 0;     // init success
      } else if (res->pkt_type != HELLO_PKT || res->addr != fixed_id) {    // expect hello packet
        return false;   // failed
      }

      return true;    // init success, prepare for paring
    }

    bool pair() {
      if (curChannel < 255) return true;   // no need to pair

      Serial.println("start pairing...");

      resetSettings(1);

      PairStartPkt startpkt;
      for (u8 i = 0; i < 5; ++i) {
        transmit(BIND_CHANNEL, (uint8_t*)&startpkt);
      }

      SyncTestPkt testPkt;
      for (int i = 49; i >= 0; --i) { // start from the biggest
        transmit(pgm_read_byte_near(&hop_data[i]), (uint8_t*)&testPkt);
        _delay_us(3000);
        printlog(0, "sent on %d", i);
      }

      printlog(0, "end wait...");
      if (!receive(packet_buff, 200000)) {
        return false;
      }
      WelcomebackPkt* fin = (WelcomebackPkt*) &packet_buff[0];
      if (fin->pkt_type != WELCOMEBACK_PKT || fin->addr != fixed_id) {
        return false; //pair failed
      }
      memcpy(hop_channels, fin->paired_channels, HOP_CH);
      curChannel = 0;     // pairing success
      resetSettings(0);
      Serial.println("pair done");
      for (int i = 0 ; i < HOP_CH; ++i) {
        printlog(0, "%X ", hop_channels[i]);
      }

      return true;
    }

    /**
     * transmit & receive using channel hopping:
     *
     *  channel  1            2             3             4             5             6             7             8
     *  receiver |--rs-----w-|---rs------w-|---rs------w-|-----------w-|-----------w-|-----------w-|---rs------w-|------
     *  sender   |ss------r----|ss-----r-----|ss----xxxxxx-|ss-----------|ss-----------|ss-----------|ss-------r---|ss----
     *  channel  1              2             3             4             5             6             7             8
     */
    u32 transmitAndReceive() {
      u32 startFrame = micros();    // mark new frame
      if (curChannel == 255) return 0; // not ready to work

      Serial.println(startFrame);
//      if (startFrame < 20000000 || (startFrame > 50000000 && startFrame < 90000000) || startFrame > 120000000 || curChannel == 5){
        buildDataPacket(packet_buff);
        transmit(hop_channels[curChannel], packet_buff);
//      } else {
//        _delay_us(7000);
//      }

      if (curChannel == 2 || curChannel == 9) {
        // telemetry packet is 9 bytes length
        if (receive(packet_buff, 6000, 8) && packet_buff[1] == fixed_id && packet_buff[2] == TELE_PKT) {
          memcpy(&receiverStatus, packet_buff, sizeof(ReceiverStatusPkt));
          receiverStatus.teleLastReceived = startFrame;

//          receiverStatus.lqi = 2;
//          Serial.println(micros() - startFrame);
//          Serial.println(receiverStatus.battery);
          Serial.println(receiverStatus.packetLost);
          printlog(1, "%d %d", receiverStatus.lqi >> 7, receiverStatus.lqi & CC2500_LQI_EST_BM);
          Serial.println(receiverStatus.rssi);
        }

        // the receiving of telemetry take extra ~7ms
        // so we increase timeframe to 21ms (14+7 = 21)
        startFrame += 7000;
      }

      curChannel = ++curChannel % HOP_CH;
      return startFrame + TRANSMISSION_INTERVAL;
    }

    // set channel data
    void setChannelValue(uint8_t channel, uint16_t value) {
      channel_data[channel] = value;
    }

    uint16_t getChannelValue(uint8_t channel) {
      return channel_data[channel];
    }

    boolean badSignal() {
      // <=127 mean transmitter & receiver is too near. See datasheet
      return (u8)(receiverStatus.lqi & CC2500_LQI_EST_BM) > 93 && receiverStatus.rssi < -88;
    }

  private:
    uint8_t fixed_id = GLOBAL_CFG.moduleId;
    uint8_t hop_channels[HOP_CH];
    uint8_t curChannel = 255;   // 255 mean not successfully paired
    int16_t channel_data[11];  // store up to 11 channels
    uint8_t packet_buff[MAX_PKT];   // buffer for building/storing temporary packet

    void transmit(uint8_t channel, uint8_t* buff) {
      CC2500_Strobe(CC2500_SIDLE);  // exit RX mode
      CC2500_WriteReg(CC2500_0A_CHANNR, channel);
      CC2500_SetTxRxMode(TX_EN);
      
      buff[0] = FIXED_PKT_LEN;
      buff[1] = fixed_id; // auto append address

      u16 crc = crc16(buff, FIXED_PKT_LEN - 2); // exclude two last byte which contains CRC check;
      
      buff[FIXED_PKT_LEN - 2] = crc >> 8;
      buff[FIXED_PKT_LEN - 1] = crc;

      CC2500_WriteData(buff, FIXED_PKT_LEN);  // write 25 bytes data took ~5ms
      _delay_us(5000);  // wait for transmission complete
      CC2500_SetTxRxMode(TXRX_OFF);
    }

    // timeout in microseconds
    uint8_t receive(uint8_t* buffer, uint32_t timeout, u8 pktLen = FIXED_PKT_LEN) {
      CC2500_Strobe(CC2500_SIDLE);  // init (not sure)
      CC2500_SetTxRxMode(RX_EN);    // set GDO0/GDO2 depend on tx or rx mode
      CC2500_Strobe(CC2500_SFRX);   // flush receive buffer
      CC2500_Strobe(CC2500_SRX);    // RX: enable RX
      uint8_t len;
      uint32_t time_start = micros();
      uint32_t time_exec = 0;
      do {
        _delay_us(1000);
        len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST);
        if (len && len <= FIXED_PKT_LEN) {
          CC2500_ReadData(buffer, pktLen);  // read 25 bytes data took ~5ms
          CC2500_SetTxRxMode(TXRX_OFF);
          return len;
        }
        time_exec = micros() - time_start;
      } while (time_exec < timeout);

      CC2500_SetTxRxMode(TXRX_OFF);
      return 0;
    }

    void buildDataPacket(uint8_t* sbus_data_pkt) {
      u8 offset = 3;  // 3 bytes header
      
      sbus_data_pkt[0] = 16 + offset;
      sbus_data_pkt[1] = fixed_id;
      sbus_data_pkt[2] = DATA_PKT;

      // reset counters
      uint8_t ch = 0;
      uint8_t bit_in_channel = 0;
      uint8_t byte_in_sbus = offset;
      uint8_t bit_in_sbus = 0;

      // store servo data
      for (u8 i = 0; i < 121; i++) { // 121 = SBUS Word * number of channels
        if (channel_data[ch] & (1 << bit_in_channel)) {
          sbus_data_pkt[byte_in_sbus] |= (1 << bit_in_sbus);
        }
        bit_in_sbus++;
        bit_in_channel++;

        if (bit_in_sbus == 8) {
          bit_in_sbus = 0;
          byte_in_sbus++;
          sbus_data_pkt[byte_in_sbus] = 0;
        }
        if (bit_in_channel == 11) {
          bit_in_channel = 0;
          ch++;
        }
      }
    }

    void resetSettings(uint8_t bind) {
      CC2500_WriteReg(CC2500_17_MCSM1, 0x00); //CCA_MODE = 0 (Always) / RXOFF_MODE = 0 / TXOFF_MODE = 0 (automatically switch to IDLE)
      CC2500_WriteReg(CC2500_18_MCSM0, 0x18); //FS_AUTOCAL = 0x01 / PO_TIMEOUT = 0x10 ( Expire count 64 Approx. 149 – 155 µs)
      CC2500_WriteReg(CC2500_06_PKTLEN, FIXED_PKT_LEN); //PKTLEN=0x19 (25 bytes)
      CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04); //PQT = 0  / CRC_AUTOFLUSH = 0 / APPEND_STATUS = 1 / ADR_CHK = 00 (Address check 0x00 broadcast)
      CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x01); //WHITE_DATA = 0 / PKT_FORMAT = 0 / CC2400_EN = 0 / CRC_EN = 0 / LENGTH_CONFIG = 1 (variable length)
      CC2500_WriteReg(CC2500_3E_PATABLE, 0xff); //PATABLE(0) = 0xFF (+1dBm)
      CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x0A); //FREQ_IF = 253.906kHz
      CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00); //FREQOFF = 0
      CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c);
      CC2500_WriteReg(CC2500_0E_FREQ1, 0x76);
      CC2500_WriteReg(CC2500_0F_FREQ0, 0x27); //FREQ = 0x5C7627 (F = 2404MHz)
      CC2500_WriteReg(CC2500_10_MDMCFG4, 0x7B); //CHANBW_E = 1 / CHANBW_M = 3 / BW = 232.143kHz / DRATE_E = 0x0B
      CC2500_WriteReg(CC2500_11_MDMCFG3, 0x51); //DRATE_M = 0x61 Bitrate = 67047.11914 bps
      CC2500_WriteReg(CC2500_12_MDMCFG2, 0x13); //MOD_FORMAT = 0x01 (GFSK) / SYNC_MODE = 3 (30/32 sync word bits detected)
      CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23); //FEC_EN = Disable / NUM_PREAMBLE = 0x02 (4 bytes) / CHANSPC_E = 0x03
      CC2500_WriteReg(CC2500_14_MDMCFG0, 0x7a); //CHANSPC_M = 0x7A Channel Spacing = 299927Hz
      CC2500_WriteReg(CC2500_15_DEVIATN, 0x51); //DEVIATION_E = 5 / DEVIATION_M = 1 / Deviation = 57129Hz
      CC2500_WriteReg(CC2500_19_FOCCFG, 0x16); //FOC_BS_CS_GATE = 0 / FOC_PRE_K = 0x10 / FOC_POST_K = 1 / FOC_LIMIT = 0x10
      CC2500_WriteReg(CC2500_1A_BSCFG, 0x6c); //BS_PRE_KI = 0x01 (2KI) / BS_PRE_KP = 0x10 (3KP) / BS_POST_KI = 1 (KI /2) / BS_POST_KP = 1 (Kp) / BS_LIMIT = 0
      CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x03); //MAX_DVGA_GAIN = 0 / MAX_LNA_GAIN = 0 / MAGN_TARGET = 0x011
      CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x40);
      CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0x91);
      CC2500_WriteReg(CC2500_21_FREND1, 0x56);
      CC2500_WriteReg(CC2500_22_FREND0, 0x10); //LODIV_BUF_CURRENT = 1
      CC2500_WriteReg(CC2500_23_FSCAL3, 0xa9);
      CC2500_WriteReg(CC2500_24_FSCAL2, 0x0a);
      CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);
      CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);
      CC2500_WriteReg(CC2500_29_FSTEST, 0x59); //(Same as specified in datasheet)
      CC2500_WriteReg(CC2500_2C_TEST2, 0x88); //(Same as specified in datasheet and by SmartRF sw)
      CC2500_WriteReg(CC2500_2D_TEST1, 0x31); //(Same as specified in datasheet and by SmartRF sw)
      CC2500_WriteReg(CC2500_2E_TEST0, 0x0b); //0x0B (Same as specified in datasheet and by SmartRF sw)
      CC2500_WriteReg(CC2500_03_FIFOTHR, 0x07);
      CC2500_WriteReg(CC2500_09_ADDR, 0x00);      // broadcast address
      CC2500_SetPower(bind ? CC2500_BIND_POWER : CC2500_MAX_POWER);
      CC2500_WriteReg(CC2500_0A_CHANNR, BIND_CHANNEL);

      CC2500_Strobe(CC2500_SIDLE);    // Go to idle...
    }
};

#endif
