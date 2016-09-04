#if not defined(__TWO_WAY_SYNC_PROTOCOL_H__)
#define __TWO_WAY_SYNC_PROTOCOL_H__

#include <avr/pgmspace.h>

#include "protocol.h"
#include "CC2500_SPI.h"
#include "crc.h"

#define DEFAULT_ID        0x01

#define BIND_CHANNEL      0x4A

#define HOP_CH            15    // Number of channels for hopping

#define FIXED_PKT_LEN     25    // fixed radio packet length
#define MAX_PKT           27    // radio config length(25) + rssi(1) + lqi status(1) = 27

#define PKT_HEAD          235

#define HELLO_PKT         255
#define WELCOMEBACK_PKT   253
#define TEST_PKT          252
#define PAIR_PKT          251
#define DATA_PKT          11
#define TELE_PKT          12

typedef struct HelloPkt {
  uint8_t len = sizeof(HelloPkt);
  uint8_t addr;
  uint8_t pkt_type = HELLO_PKT;
  uint8_t padding[FIXED_PKT_LEN - 3]; // remaining bytes to fit FIXED_PKT_LEN
} HelloPkt;

typedef struct WelcomebackPkt {
  uint8_t len = sizeof(WelcomebackPkt);
  uint8_t addr;
  uint8_t pkt_type = WELCOMEBACK_PKT;
  uint8_t paired_channels[HOP_CH];
  uint8_t padding[FIXED_PKT_LEN - HOP_CH - 3]; // remaining bytes to fit FIXED_PKT_LEN
} WelcomebackPkt;

typedef struct PairStartPkt {
  uint8_t len = sizeof(PairStartPkt);
  uint8_t addr;
  uint8_t pkt_type = PAIR_PKT;
  uint8_t padding[FIXED_PKT_LEN - 3]; // remaining bytes to fit FIXED_PKT_LEN
} PairStartPkt;

typedef struct SyncTestPkt {
  uint8_t len = sizeof(SyncTestPkt);
  uint8_t addr;
  uint8_t pkt_type = TEST_PKT;
  uint8_t lqi = 0;
  uint8_t padding[FIXED_PKT_LEN - 4]; // remaining bytes to fit FIXED_PKT_LEN
} SyncTestPkt;

typedef struct ReceiverStatusPkt {
  uint8_t len = sizeof(ReceiverStatusPkt);
  uint8_t addr;
  uint8_t pkt_type = TELE_PKT;
  uint8_t packetLost = 0;
  uint8_t lqi = 0;
  uint8_t rssi = 0; 
  uint16_t battery = 800; // min battery
  // end transmit params
  uint16_t cycleCount = 0;
  uint16_t error_pkts = 0;        // count number of error packets
  u32 lastReceived;               // last timestamp packet was received
  uint8_t padding[FIXED_PKT_LEN - 16]; // remaining bytes to fit FIXED_PKT_LEN
} ReceiverStatusPkt;

enum {
  PAIRING = 1,
  TRANSMISSION = 2,
  RADIO_LOST = 3
};

#define ACCEPTABLE_LQI    126

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

uint8_t lq_table[50]; // lqi table for each channel

class TwoWaySyncProtocol {
  public:
    ReceiverStatusPkt stats;

    // will be run when device startup
    void init() {
      Serial.println("init");
      CC2500_Reset();
      delay(500);    // wait for module to fully reset and stable

      resetSettings(1); // startup in bind stage

      Serial.println("listen...");
      do {

        uint8_t arr[] = {CC2500_10_MDMCFG4, CC2500_11_MDMCFG3, CC2500_12_MDMCFG2, CC2500_13_MDMCFG1, CC2500_14_MDMCFG0, CC2500_15_DEVIATN, CC2500_06_PKTLEN, CC2500_08_PKTCTRL0};
        for (int i = sizeof(arr) - 1; i >= 0; --i) {
          printlog(0, "0x%X: 0x%X", arr[i], CC2500_ReadReg(arr[i]));
        }
        printlog(0, "try again %d", stats.error_pkts);
        if (receive(BIND_CHANNEL, 3000000)) {
          HelloPkt* hello = (HelloPkt*)&packet_buff[0];
          if (hello->pkt_type == HELLO_PKT) { // validate
            Serial.println("response to hello");
            fixed_id = hello->addr;
            for (int i = 0; i < 5; ++i) {
              _delay_us(2000);
              transmit((uint8_t*)hello);
            }
            state = PAIRING;
          } else {
            Serial.println("invalid pkt");
          }
        }
      } while (state == 0);
    }

    /**
     * transmit & receive using channel hopping:
     *
     * channel  1            2             3             4             5             6             7             8
     * receiver |--rs-----w-|---rs------w-|---rs------w-|-----------w-|-----------w-|-----------w-|---rs------w-|------
     * sender   |ss------r----|ss-----r-----|ss----xxxxxx-|ss-----------|ss-----------|ss-----------|ss-------r---|ss----
     * channel  1              2             3             4             5             6             7             8
     */
    bool receiveData(int32_t* delay) {
      *delay = 0; // default

      if (state == PAIRING) {
        if (pair()) {
          // wait for first packet to end pairing stage
          while (!receive(hop_channels[curChannel], 1000000));
          if (packet_buff[2] != DATA_PKT || packet_buff[1] != fixed_id) {
            state = RADIO_LOST;
            return false;
          }
          curChannel = ++curChannel % HOP_CH;
          *delay = micros() + 6000;
          Serial.println("start transmission");
        }
      } else if (state == TRANSMISSION) {
        u32 startFrame = micros();  // start timeframe
        u32 delayTime = 14500;  // delay 14ms (transmitter interval) when packet not received. Actual value measured is about 14504us
        Serial.println(startFrame);
        if (receive(hop_channels[curChannel], 7000) && packet_buff[2] == DATA_PKT && packet_buff[1] == fixed_id) {
          startFrame = stats.lastReceived;  // update timeframe using packet receiving time,
                                            // receiving process take about 6ms if success.
          delayTime = 6500;     // Delay total ~14ms if packet successfully received
                                // Actual value measured is ~14520us (TODO: why it's higher than expected???)
          
          lq_table[curChannel] = stats.lqi;
          
          Serial.println(hop_channels[curChannel], HEX);
          // Response telemetry every 2nd & 9th cycle of channel hoping freq
          // This is to save power & improve performance
          if (curChannel == 2 || curChannel == 9) {
            transmit((uint8_t*)&stats, 8, false); // telemetry packet doesn't need to send full length
          }
          stats.packetLost = 0;
        } else {
//          Serial.println(hop_channels[curChannel], HEX);
          ++stats.packetLost;
          if (isRadioLost()) { // 13 * 100 = 1300: radio lost timeout
            state = RADIO_LOST;
          }
        }

        // the transmission of telemetry take extra 7ms
        if (curChannel == 2 || curChannel == 9) startFrame += 7000;

        curChannel = ++curChannel % HOP_CH;
        *delay = startFrame + delayTime;
        
        return stats.packetLost == 0;
      } else if (state == RADIO_LOST) {
        if (curChannel != 255) {
          Serial.println("lost");
          curChannel = 255;
          resetSettings(0);
        }
        waitForSignal();
      }

      return false;
    }

    bool isRadioLost() {
      return stats.packetLost > 100;
    }

    void buildSbusPacket(uint8_t* sbus_data) {
      memcpy(&sbus_data[1], &packet_buff[3], 16);   // 16 = SBUS WORD * number of channels(11)
    }

  private:
    uint8_t fixed_id = 0;           // not initialized. Value will be set when pairing
    uint8_t hop_channels[HOP_CH];   // channels for frequency hoping
    uint8_t curChannel = 255;       // 255 mean not successfully paired
    uint8_t packet_buff[MAX_PKT];   // received data is stored here
    uint8_t state = 0;              // current state (PAIR, TRANSMISSION, RADIO_LOST)

    void waitForSignal() {
      if (receive(BIND_CHANNEL, 100000)) {
        HelloPkt* hi = (HelloPkt*)&packet_buff[0];
        if (hi->pkt_type == HELLO_PKT && hi->addr == fixed_id) {
          printlog(0, "resume");
          WelcomebackPkt res;
          memcpy(res.paired_channels, hop_channels, HOP_CH);
          transmit((uint8_t*)&res);

          curChannel = 0;   // pairing success
          state = TRANSMISSION;
        }
      }
    }

    bool pair() {
      Serial.println("listen to pair...");
      resetSettings(1);

      while (receive(BIND_CHANNEL, 200000) == false);

      PairStartPkt* res = (PairStartPkt*)&packet_buff[0];
      if (res->pkt_type != PAIR_PKT || res->addr != fixed_id) {
        return false;
      }

      Serial.println("start pairing...");

      int chanCounter = 50;  // hop_data len
      u8 chann = 0;
      u8 bestLqi = ACCEPTABLE_LQI; // worst LQI value allowed
      do {
        --chanCounter;
        chann = pgm_read_byte_near(&hop_data[chanCounter]);
        if (!receive(chann, 200000) || stats.lqi > ACCEPTABLE_LQI) {
          printlog(0, "failed at %d", chanCounter);
          return false;
        }

        lq_table[chanCounter] = stats.lqi;
        if (lq_table[chanCounter] < bestLqi) bestLqi = lq_table[chanCounter];
        printlog(0, "%d lqi: %d", chann, lq_table[chanCounter]);
      } while (chanCounter > 0);

      WelcomebackPkt fin;
      getHopChannels(fin.paired_channels, lq_table, bestLqi);

      memcpy(hop_channels, fin.paired_channels, HOP_CH);
      for (u8 i = 0; i < 2; ++i) {
        _delay_us(2000);
        transmit((uint8_t*)&fin);
      }
      curChannel = 0;   // pairing success
      state = TRANSMISSION;

      // get ready for transmission state
      resetSettings(0);
      Serial.println("ready");
      return true;
    }

    void getHopChannels(u8* output, u8* lqiTable, u8 minLqi) {
      u8 remain = HOP_CH;
      u8 nextLqi = ACCEPTABLE_LQI;
      while(remain > 0) {
        for (u8 i = 0; i < 50 && remain > 0; ++i) {
          if (lqiTable[i] == minLqi) {
            output[--remain] = pgm_read_byte_near(&hop_data[i]);
          } else if (lqiTable[i] > minLqi && lqiTable[i] < nextLqi) {
            nextLqi = lqiTable[i];
          }
        }
        minLqi = nextLqi;
        nextLqi = ACCEPTABLE_LQI;
      }
    }

    void transmit(uint8_t* buff, u8 len = FIXED_PKT_LEN, bool wait = true) {
      buff[0] = len;
      buff[1] = fixed_id; // auto append address

      CC2500_Strobe(CC2500_SIDLE);  // exit RX mode
      CC2500_SetTxRxMode(TX_EN);
      CC2500_WriteData(buff, len);  // write 25 bytes data took ~5ms
      if (wait) {
        _delay_us(5000);  // wait for transmission complete
        CC2500_SetTxRxMode(TXRX_OFF);
      }
    }

    uint8_t receive(uint8_t channel, uint32_t timeout) { // timeout in microseconds
      CC2500_Strobe(CC2500_SIDLE);  // exit TX mode
      CC2500_WriteReg(CC2500_0A_CHANNR, channel);
      CC2500_SetTxRxMode(RX_EN);
      CC2500_Strobe(CC2500_SFRX);   // flush receive buffer
      CC2500_Strobe(CC2500_SRX);
      uint8_t len;
      uint32_t time_start = micros();
      uint32_t time_exec = 0;
      do {
        _delay_us(500);
        len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST);
        // Read FIFO continuously until we found expected packet (RXOFF_MODE = 11)
        if (len && len <= FIXED_PKT_LEN) {
          CC2500_ReadData(packet_buff, MAX_PKT);  // read 25 bytes data took ~5ms

          stats.lastReceived = micros();
          stats.lqi = packet_buff[MAX_PKT - 1];
          stats.rssi = packet_buff[MAX_PKT - 2];

          CC2500_SetTxRxMode(TXRX_OFF);
          CC2500_Strobe(CC2500_SIDLE);  // IDLE once done
          if (crcCheck(packet_buff)) {
            return len;
          } else {
            ++stats.error_pkts;
            return 0;
          }
        }
        time_exec = micros() - time_start;
      } while (time_exec < timeout);

      CC2500_SetTxRxMode(TXRX_OFF);
      CC2500_Strobe(CC2500_SIDLE);  // IDLE once done
      return 0;
    }

    bool crcCheck(u8* raw_pkt) {
      // lazy packet integrity check
      return (u8)(~raw_pkt[3]) == raw_pkt[FIXED_PKT_LEN - 1];
    }

    void resetSettings(uint8_t bind) {
      CC2500_WriteReg(CC2500_17_MCSM1, 0x00); //CCA_MODE = 0 (Always) / RXOFF_MODE = 0 / TXOFF_MODE = 0
      CC2500_WriteReg(CC2500_18_MCSM0, 0x18); //FS_AUTOCAL = 0x01 / PO_TIMEOUT = 0x10 ( Expire count 64 Approx. 149 – 155 µs)
      CC2500_WriteReg(CC2500_06_PKTLEN, FIXED_PKT_LEN); //PKTLEN=0x19 (25 bytes)
      CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04); //PQT = 0  / CRC_AUTOFLUSH = 0 / APPEND_STATUS = 1 / ADR_CHK = 00 (no address check)
      CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x01); //WHITE_DATA = 0 / PKT_FORMAT = 0 / CC2400_EN = 0 / CRC_EN = 0 / LENGTH_CONFIG = 01 (variable length)
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
      CC2500_SetPower(bind ? CC2500_BIND_POWER : CC2500_HIGH_POWER);
      CC2500_WriteReg(CC2500_0A_CHANNR, BIND_CHANNEL);

      CC2500_Strobe(CC2500_SIDLE);    // Go to idle...
    }
};

#endif
