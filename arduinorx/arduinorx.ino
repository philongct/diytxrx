//this programm will put out a PPM signal

/*
 * 
 * inputs:
 * A1-A4: Analog (battery monitor)
 * D2-D3: sbus communication
 * 
 * SPI pins:
 * D5: CE
 * D6: CSN
 * D11: MOSI
 * D12: MISO
 * D13: SCK
 */

#define SBUS_TX   2
#define SBUS_RX   3
#define RF_CE     5
#define RF_CSN    6

#include <SPI.h>
#include <RF24.h>

#include "SoftSerial.h"
#include "FUTABA_SBUS.h"

SoftSerial invertedConn(SBUS_TX, SBUS_TX, true);
FUTABA_SBUS sbus(&invertedConn);

RF24Debug radio(RF_CE, RF_CSN);
const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };

void setup(){
  delay(3000); // For programming
  
  Serial.begin(57600);
  sbus.begin();

  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.openWritingPipe(pipes[1]);    // note that our pipes are the same above, but that
  radio.openReadingPipe(1, pipes[0]); // they are flipped between rx and tx sides.
  radio.startListening();
  
  radio.printDetails();
}

// the loop routine runs over and over again forever:
void loop() {

  if (radio.available()) {

    Serial.println("--------------------------------------------------------------------------------");
    uint8_t rx_data[32];  // we'll receive a 32 byte packet
    
    bool done = false;
    while (!done) {
      done = radio.read( &rx_data, sizeof(rx_data) );
      printf("Got payload @ %lu...\r\n", millis());
    }
    
    // echo it back real fast
    radio.stopListening();
    radio.write( &rx_data, sizeof(rx_data) );
    Serial.println("Sent response.");
    radio.startListening();

    // do stuff with the data we got.
    Serial.print("First Value: ");
    Serial.println(rx_data[0]);
  }

  
  
  static int val = 10;

  for (uint8_t i = 5; i < 15; ++i) {
    sbus.setChannelData(i, sbus.getChannelData(i) + val);
    if(sbus.getChannelData(i) >= 2000){ val = -10; }
    if(sbus.getChannelData(i) <= 1000){ val = 10; }
  }

  sbus.write();
}
