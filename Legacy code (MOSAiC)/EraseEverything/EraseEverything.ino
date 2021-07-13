//#include <RHReliableDatagram.h>
//#include <RH_RF95.h>
#include <SerialFlash.h>
#include <SPI.h>

#define Serial SerialUSB
//#define CLIENT_ADDRESS 1
//#define SERVER_ADDRESS 2

//// Define pins
//#define RFM95_CS 5
//#define RFM95_INT 2
//#define LED 13
//
//// Define frequency
//#define RF95_FREQ 915.0
//
//// Singleton instance of the radio driver
//RH_RF95 driver(RFM95_CS, RFM95_INT);
//
//// Class to manage message delivery and receipt, using the driver declared above
//RHReliableDatagram manager(driver, CLIENT_ADDRESS);

const int FlashChipSelect = 4; // digital pin for flash chip CS pin
//const int FlashChipSelect = 21; // Arduino 101 built-in SPI Flash

SerialFlashFile file;

const unsigned long testIncrement = 4096;

void setup() {
  //uncomment these if using Teensy audio shield
  //SPI.setSCK(14);  // Audio shield has SCK on pin 14
  //SPI.setMOSI(7);  // Audio shield has MOSI on pin 7

  //uncomment these if you have other SPI chips connected
  //to keep them disabled while using only SerialFlash
  pinMode(5, INPUT_PULLUP);
  //pinMode(10, INPUT_PULLUP);

  Serial.begin(9600);

  // wait up to 10 seconds for Arduino Serial Monitor
  unsigned long startMillis = millis();
  while (!Serial && (millis() - startMillis < 10000)) ;
  delay(100);

  SerialFlash.begin(FlashChipSelect);
  SerialFlash.wakeup();

//  if (!SerialFlash.begin(FlashChipSelect)) {
//    while (1) {
//      Serial.println("Unable to access SPI Flash chip");
//        SerialFlash.wakeup();
//        delay(1000);
//    }
//  }
  unsigned char id[5];
  SerialFlash.readID(id);
  Serial.println(id[0]);
  unsigned long size = SerialFlash.capacity(id);

  if (size > 0) {
    Serial.print("Flash Memory has ");
    Serial.print(size);
    Serial.println(" bytes.");
    Serial.println("Erasing ALL Flash Memory:");
    // Estimate the (lengthy) wait time.
    Serial.print("  estimated wait: ");
    int seconds = (float)size / eraseBytesPerSecond(id) + 0.5;
    Serial.print(seconds);
    Serial.println(" seconds.");
    Serial.println("  Yes, full chip erase is SLOW!");
    SerialFlash.eraseAll();
    unsigned long dotMillis = millis();
    unsigned char dotcount = 0;
    while (SerialFlash.ready() == false) {
      if (millis() - dotMillis > 1000) {
        dotMillis = dotMillis + 1000;
        Serial.print(".");
        dotcount = dotcount + 1;
        if (dotcount >= 60) {
          Serial.println();
          dotcount = 0;
        }
      }
    }
    if (dotcount > 0) Serial.println();
    Serial.println("Erase completed");
    unsigned long elapsed = millis() - startMillis;
    Serial.print("  actual wait: ");
    Serial.print(elapsed / 1000ul);
    Serial.println(" seconds.");
  }
}

float eraseBytesPerSecond(const unsigned char *id) {
  if (id[0] == 0x20) return 152000.0; // Micron
  if (id[0] == 0x01) return 500000.0; // Spansion
  if (id[0] == 0xEF) return 419430.0; // Winbond
  if (id[0] == 0xC2) return 279620.0; // Macronix
  return 320000.0; // guess?
}


void loop() {

}
