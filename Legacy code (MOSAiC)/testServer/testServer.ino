/* I AM TESTING
   testServer.ino

   basic server for receiving data from readWriteSleepTransmit, printing data, and
   saving to files

   Ian Raphael 06/29/19
   ianaraphael@gmail.com

*/

/************ INCLUDES ************/
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SerialFlash.h>

/************ MACROS ************/
#define Serial SerialUSB // usb port
#define CLIENT_ADDRESS 1 // address for sender (distributed client/loggers)
#define SERVER_ADDRESS 2 // address for receiver (central server/base stn.)
#define RFM95_CS 5 // radio pin
#define RFM95_INT 2 // radio pin
#define RF95_FREQ 915.0 // radio frequency
#define LED 13

/************ GLOBALS ************/
const int FlashChipSelect = 4; // flash chip enable

// File stuff
int numSensors = 5;
int writeBufSize = 2*numSensors; // size of the buffer for writing temp to file
int readBufSize = 2*numSensors; // size of the buffer for reading temp off of file
int fileSize = 2*numSensors; // size of the file for holding temps

// Radio
RH_RF95 driver(RFM95_CS, RFM95_INT); // radio driver
RHReliableDatagram manager(driver, SERVER_ADDRESS); // delivery/receipt manager
uint8_t msgBuf[RH_RF95_MAX_MESSAGE_LEN]; // buf for TX/RX handshake messages
byte msg[RH_RF95_MAX_MESSAGE_LEN];
byte snr[2];


/************ setup() ************/
void setup() {


  /************ Board setup ************/

  unsigned char pinNumber;
  // Pullup all unused pins to prevent floating vals
  for (pinNumber = 0; pinNumber < 23; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  for (pinNumber = 32; pinNumber < 42; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(LED, HIGH);

  /************ Flash init ************/

  // wake up the flash
  SerialFlash.begin(FlashChipSelect);
  SerialFlash.wakeup();

  // If connection to flash fails
  if (!SerialFlash.begin(FlashChipSelect)) {

    // print error
    Serial.println("Unable to access SPI Flash chip");
    while (1);
  }

  // otherwise print success
  Serial.println("Able to access SPI flash chip");

  // print chip capacity
  unsigned char id[5];
  SerialFlash.readID(id);
  unsigned long size = SerialFlash.capacity(id);
  Serial.print("Flash memory capacity (bytes): ");
  Serial.println(size);

  // erase everything on the chip
  Serial.println("Erasing everything:");
  SerialFlash.eraseAll();
  Serial.println("Done erasing everything");

  SerialFlash.sleep();


  /************ USB monitor init ************/
  while (!Serial) ; // Wait for serial port to be available
  Serial.begin(9600);
  Serial.println("Server online");
  delay(100);


  // if failure to init the manager
  if (!manager.init()) {

    // print error
    Serial.println("Unable to init server radio");
    while (1);
  }
  // otherwise print success
  Serial.println("Server radio online");

  // set parameters
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
}


void loop() {

  if (manager.available())
  {
    Serial.println("Waiting for message");

    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(msgBuf);
    uint8_t from;

    if (manager.recvfromAck(msgBuf, &len, &from))
    {
      Serial.print("got request from : 0x");
      Serial.print(from, HEX);
      Serial.println(": ");

      // print the received buffer
      Serial.print("Received buffer: ");
      for (int l=0;l<len;l++) {
        Serial.print(msgBuf[l]);
        Serial.print(" ");
      }
      Serial.println();

      // get the size of the filename
      int filenameSize = msgBuf[0];

      // print it
      Serial.print("Filename size: ");
      Serial.println(filenameSize,DEC);
      
      // get the filename
      char filename[filenameSize+1];
      int i;
      for (i=1;i<filenameSize+1;i++) {
        filename[i-1] = msgBuf[i];
      }
      filename[filenameSize] = '\0';
      Serial.print("  Parsed filename: ");
      Serial.println((char *)filename);

      // get the number of temp readings
      int nTemps = (msgBuf[i]/2) - 2;
      i++;

      // print it
      Serial.print("number of temps: ");
      Serial.println(nTemps,DEC);

      // init a buffer to hold temps
      int tempArray[nTemps];
      
      // for every temperature
      for (int j=0;j < nTemps;j++) {
        
        // get a temp
        tempArray[j] = word(msgBuf[i],msgBuf[i+1]);
        
        // increment i twice (read two bytes)
        i+=2;
      }

      // get the pinger value
      int pingerOutput = word(msgBuf[i],msgBuf[i+1]);
      i+=2;

      short adcRead = word(msgBuf[i],msgBuf[i+1]);
      Serial.println(adcRead,DEC);
      
      float battVoltage = ((float)  adcRead) * 1023.0;
      
      Serial.print("  Parsed temps: ");
      for (i=0;i<nTemps;i++) {
        Serial.print(tempArray[i], DEC);
        Serial.print(" ");
      }
      Serial.println("");

      Serial.print("  Parsed pinger value: ");
      Serial.println(pingerOutput,DEC);

      Serial.print("  Parsed battery voltage: ");
      Serial.println(battVoltage, 2);

      // Send a reply back to the originator client
      if (!manager.sendtoWait((uint8_t*)msgBuf, sizeof(msgBuf), from))
      {
        digitalWrite(LED, LOW);
        delay(50);
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
        delay(50);
        digitalWrite(LED, HIGH);
      }
    }
  }
}
