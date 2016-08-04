#if not defined(__TWO_WAY_SYNC_PROTOCOL_H__)
#define __TWO_WAY_SYNC_PROTOCOL_H__

#include "protocol.h"
#include "CC2500_SPI.h"

#define DEFAULT_ID        0x01

#define BIND_CHANNEL      0x05

#define HOP_CH            15    // Number of channels for hopping

#define MAX_PKT           28    // length prefix(1) + radio config length(25) + rssi(1) + crc status(1) = 28
#define PKT_HEAD          235

#define HELLO_PKT         255
#define WELCOMEBACK_PKT   253
#define TEST_PKT          252
#define DATA_PKT          11

typedef struct HelloPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = HELLO_PKT;
  uint8_t addr;
} HelloPkt;

typedef struct WelcomebackPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = WELCOMEBACK_PKT;
  uint8_t addr;
  uint8_t paired_channels[HOP_CH];
} WelcomebackPkt;

typedef struct SyncTestPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = TEST_PKT;
  uint8_t rssi = 0;
} SyncTestPkt;

typedef struct ReceiverStatusPkt {
  uint8_t head = PKT_HEAD;
  uint8_t pkt_type = DATA_PKT;
  uint8_t packetLost = 0;
  uint8_t rssi = 0;
  uint8_t battery1 = 200;
  uint8_t battery2 = 200;
} ReceiverStatusPkt;

enum {
  PAIRING = 1,
  TRANSMISSION = 2,
  RADIO_LOST = 3
};

const PROGMEM uint8_t hop_data[] = {
  0x02,  0xD4, 0xBB, 0xA2, 0x89,
  0x70, 0x57, 0x3E, 0x25, 0x0C,
  0xDE, 0xC5, 0xAC, 0x93, 0x7A,
  0x61, 0x48, 0x2F, 0x16, 0xE8,
  0xCF, 0xB6, 0x9D, 0x84, 0x6B,
  0x52, 0x39, 0x20, 0x07, 0xD9,
  0xC0, 0xA7, 0x8E, 0x75, 0x5C,
  0x43, 0x2A, 0x11, 0xE3, 0xCA,
  0xB1, 0x98, 0x7F, 0x66, 0x4D,
  0x34, 0x1B, 0x00, 0x1D, 0x03
};

class TwoWaySyncProtocol {
  public:
    ReceiverStatusPkt stats;

    // will be run when device startup
    void init() {
      Serial.println("init");
      CC2500_Reset();
      delay(500);    // wait for module to fully reset

      resetSettings(1);

      Serial.println("listen...");
      do {

        uint8_t arr[] = {CC2500_10_MDMCFG4, CC2500_11_MDMCFG3, CC2500_12_MDMCFG2, CC2500_13_MDMCFG1, CC2500_14_MDMCFG0, CC2500_15_DEVIATN, CC2500_06_PKTLEN, CC2500_08_PKTCTRL0};
        for (int i = sizeof(arr) - 1; i >= 0; --i) {
          printlog(0, "0x%X: 0x%X", arr[i], CC2500_ReadReg(arr[i]));
        }
        printlog(0, "try again %d", error_pkts);
        if (receive(packet_buff, 10000000)) {
          HelloPkt* hello = (HelloPkt*)&packet_buff[1];   // first byte is data packet len
          if (hello->head == PKT_HEAD && hello->pkt_type == HELLO_PKT) { // validate
            transmit(0x00, (uint8_t*)hello, sizeof(HelloPkt));
            fixed_id = hello->addr;
            state = PAIRING;
          } else {
            Serial.println("invalid pkt");
          }
        }
      } while (state == 0);
    }

    /**
       transmit & receive process using channel hopping:

                1            2             3             4             5             6             7             8
       receiver |--rws-------|---rws-------|---rws-------|-----------w-|-----------w-|------------w|---rws-------|------
       sender   |ss---r-------|ss-----r-----|ss----xxxxxx-|ss-----------|ss-----------|ss-----------|ss---r-------|ss----
                1               2             3             4             5             6             7             8

       receiver interval after receiving packet: 9
       receiver interval if not receiving any packet: 13
       sender interval: 13
    */
    bool receiveData() {
      if (state == PAIRING) {
        pair();
      } else if (state == TRANSMISSION && micros() - lastReceive >= 13000) {
        lastReceive = micros();
        if (receive(packet_buff, 7000) && packet_buff[1] == PKT_HEAD && packet_buff[2] == DATA_PKT) {
          lastReceive -= 4000;  // delay 9ms if packet success fully received (13 - 9 = 4)
          stats.packetLost = 0;
          transmit(hop_channels[curChannel], (uint8_t*)&stats, sizeof(ReceiverStatusPkt));
        } else {
          ++stats.packetLost;
          if (isRadioLost()) { // 13 * 100 = 1300: radio lost timeout
            state = RADIO_LOST;
          }
        }
        ++curChannel;
        return stats.packetLost == 0;
      } else if (state == RADIO_LOST) {
        if (curChannel != 255) {
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
    uint8_t fixed_id = 0;   // not init yet
    uint8_t hop_channels[HOP_CH];
    uint8_t curChannel = 255;   // 255 mean not successfully paired
    uint32_t lastReceive = 0;  // use micros for more accuracy
    uint8_t packet_buff[MAX_PKT];
    uint8_t state = 0;
    uint16_t error_pkts = 0;

    void waitForSignal() {
      if (receive(packet_buff, 12000)) {
        HelloPkt* hi = (HelloPkt*)&packet_buff[1];
        if (hi->head == PKT_HEAD && hi->pkt_type == HELLO_PKT && hi->addr == fixed_id) {
          WelcomebackPkt res;
          res.addr = fixed_id;
          memcpy(res.paired_channels, hop_channels, HOP_CH);
          transmit(0, (uint8_t*)&res, sizeof(WelcomebackPkt));

          curChannel = 0;   // pairing success
          state = TRANSMISSION;

          CC2500_WriteReg(CC2500_09_ADDR, fixed_id);
          CC2500_WriteReg(CC2500_0A_CHANNR, hop_channels[curChannel]);
        }
      }
    }

    void pair() {
      resetSettings(1);
      CC2500_WriteReg(CC2500_09_ADDR, fixed_id);

      uint8_t chan = sizeof(hop_data);
      do {
        --chan;
        CC2500_WriteReg(CC2500_0A_CHANNR, hop_data[chan]);
        if (!receive(packet_buff, 20000)) {
          return;
        }

        SyncTestPkt* testPkt = (SyncTestPkt*)&packet_buff[1]; // ignore packet length prefix
        if (testPkt->head != PKT_HEAD || testPkt->pkt_type != TEST_PKT) {
          return;
        }

        testPkt->rssi = 0;
        transmit(hop_data[chan], (uint8_t*)&testPkt, sizeof(SyncTestPkt));
      } while (chan > 0);

      WelcomebackPkt fin;
      fin.addr = fixed_id;
      for (uint8_t i = 0; i < HOP_CH; ++i) {    // TODO: use rssi to determine which is best channel
        fin.paired_channels[i] = hop_data[i];
      }

      memcpy(hop_channels, fin.paired_channels, HOP_CH);
      transmit(hop_data[chan], (uint8_t*)&fin, sizeof(WelcomebackPkt));
      curChannel = 0;   // pairing success
      state = TRANSMISSION;

      // get ready for transmission state
      resetSettings(0);
      CC2500_WriteReg(CC2500_09_ADDR, fixed_id);
      CC2500_WriteReg(CC2500_0A_CHANNR, hop_channels[curChannel]);
    }

    void transmit(uint8_t channel, uint8_t* buff, uint8_t len) {
      CC2500_SetTxRxMode(TX_EN);
      CC2500_Strobe(CC2500_SIDLE);  // exit RX mode
      CC2500_WriteReg(CC2500_0A_CHANNR, channel);
      CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
      CC2500_Strobe(CC2500_STX);
      CC2500_WriteData(buff, len);
    }

    uint8_t receive(uint8_t* buffer, uint32_t timeout) { // timeout in microseconds
      CC2500_SetTxRxMode(RX_EN);
      CC2500_Strobe(CC2500_SIDLE);  // exit TX mode
      CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
      CC2500_Strobe(CC2500_SFRX);   // flush receive buffer
      CC2500_Strobe(CC2500_SRX);
      uint8_t len;
      uint32_t time_start = micros();
      uint32_t time_exec = 0;
      do {
        _delay_us(1000);
        len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST);
        if (len && len <= MAX_PKT)
        {
          CC2500_ReadData(buffer, len);
          if (buffer[0] > 0) {
            CC2500_Strobe(CC2500_SIDLE);  // to save power since not configured auto off after rx
            return buffer[0];
          } else {
            ++error_pkts;
          }
        }
        time_exec = micros() - time_start;
      } while (time_exec < timeout);

      return 0;
    }

    void resetSettings(uint8_t bind) {
      CC2500_WriteReg(CC2500_17_MCSM1, 0x0C); //CCA_MODE = 0 (Always) / RXOFF_MODE = 0x11/ TXOFF_MODE = 0
      CC2500_WriteReg(CC2500_18_MCSM0, 0x18); //FS_AUTOCAL = 0x01 / PO_TIMEOUT = 0x10 ( Expire count 64 Approx. 149 – 155 µs)
      CC2500_WriteReg(CC2500_06_PKTLEN, 0x19); //PKTLEN=0x19 (25 bytes)
      CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x0E); //PQT = 0  / CRC_AUTOFLUSH = 1 / APPEND_STATUS = 1 / ADR_CHK = 10 (Address check 0x00 broadcast)
      CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x04); //WHITE_DATA = 0 / PKT_FORMAT = 0 / CC2400_EN = 0 / CRC_EN = 1 / LENGTH_CONFIG = 0
      CC2500_WriteReg(CC2500_3E_PATABLE, 0xff); //PATABLE(0) = 0xFF (+1dBm)
      CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x0A); //FREQ_IF = 253.906kHz
      CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00); //FREQOFF = 0
      CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c);
      CC2500_WriteReg(CC2500_0E_FREQ1, 0x76);
      CC2500_WriteReg(CC2500_0F_FREQ0, 0x27); //FREQ = 0x5C7627 (F = 2404MHz)
      CC2500_WriteReg(CC2500_10_MDMCFG4, 0x7B); //CHANBW_E = 1 / CHANBW_M = 3 / BW = 232.143kHz / DRATE_E = 0x0B
      CC2500_WriteReg(CC2500_11_MDMCFG3, 0x52); //DRATE_M = 0x61 Bitrate = 67047.11914 bps
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

      CC2500_SetTxRxMode(RX_EN);  // set GDO0/GDO2 depend on tx or rx mode
      CC2500_SetPower(bind ? CC2500_BIND_POWER : CC2500_HIGH_POWER);

      CC2500_Strobe(CC2500_SIDLE);    // Go to idle...

      CC2500_WriteReg(CC2500_0A_CHANNR, BIND_CHANNEL);
      CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
      CC2500_Strobe(CC2500_SFRX);
    }
};

#endif
