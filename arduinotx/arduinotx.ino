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

#include <SPI.h>
#include <RF24.h>

#define LED 6
#define RF_CE 9
#define RF_CSN 10

RF24 radio(RF_CE, RF_CSN);
const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };

void setup(){
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
//  radio.printDetails();
}

// the loop routine runs over and over again forever:
void loop() {
  unsigned long time = millis();

  uint8_t data[32];  // we'll transmit a 32 byte packet
  data[0] = 99;      // our first byte in the pcaket will just be the number 99.
  
  // transmit the data
  radio.stopListening();
  radio.write( &data, sizeof(data) );
  radio.startListening();

  // listen for acknowledgement from the receiver
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while (!radio.available() && ! timeout)
    if (millis() - started_waiting_at > 250 )
      timeout = true;

  if (timeout){
    Serial.println("Failed, response timed out.");
  } else {
    // the receiver is just going to spit the data back
    radio.read( &data, sizeof(data) );
    digitalWrite(LED, HIGH);
    delay(100);  // light up the LED for 100ms if it worked.
    digitalWrite(LED, LOW);
    Serial.print("Got response, round trip delay: ");
    Serial.println(millis() - started_waiting_at);
  }

  Serial.print("=> -64dBm: "); Serial.println(radio.testRPD());

  delay(1000); // wait a second and do it again. 
}
