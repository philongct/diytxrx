/*
 * 
 * A1-A4: Analog
 * D2: left button (flight mode)
 * D3: right button (AUX selection)
 * D4-D5: AUX up/down
 * D6-D7: LED status (flight mode/radio signal)
 * D8: Buzzer
 * 
 * SPI pins:
 * D9: CE
 * D10: CSN
 * D11: MOSI
 * D12: MISO
 * D13: SCK
 * 
 */

//#include <SPI.h>


#include "SBUS.h"
#include "tx.h"

#define LED           6

#define RF_CE_PIN     9
#define RF_CSN_PIN    10

TX tx(RF_CE_PIN, RF_CSN_PIN);

void setup(){
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  tx.begin();
}

// the loop routine runs over and over again forever:
void loop() {
  uint8_t data[32];  // we'll transmit a 32 byte packet
  data[0] = 99;      // our first byte in the pcaket will just be the number 99.
  tx.transmit(&data, sizeof(data));
  if (tx.receiveUntilTimeout(&data, sizeof(data))) {
    Serial.print("Data 0: ");Serial.println(data[0]);
  }
  
  delay(1000); // wait a second and do it again. 
}
