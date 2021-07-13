/*
   RWST_Pinger

   Read write sleep transmit with added snow pinger functionality.

   Ian Raphael 8/16/2019
   ianaraphael@gmail.com
*/


/************ INCLUDES ************/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTCZero.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <timestamp.h>


/************ MACROS ************/
#define Serial SerialUSB // usb port
#define pinger Serial1 // snow pinger
#define ONE_WIRE_BUS 12 // temp probe data line
#define tempPower 6 // temp probe power
#define pingerPower 7 // snow pinger power
#define CLIENT_ADDRESS 1 // address for sender (distributed client/loggers)
#define SERVER_ADDRESS 2 // address for receiver (central server/base stn.)
#define RFM95_CS 5 // radio pin
#define RFM95_INT 2 // radio pin
#define RF95_FREQ 915.0 // radio frequency


/************ GLOBALS ************/

// RTC
RTCZero rtc; // RTC object

// Temp sensors
OneWire oneWire(ONE_WIRE_BUS); // oneWire object for temp probe coms
DallasTemperature sensors(&oneWire); // temp probes coms
const int numSensors = 5; // number of temp sensors
uint8_t addr[numSensors][8]; // temp probe addresses

// Radio
RH_RF95 driver(RFM95_CS, RFM95_INT); // radio driver
RHReliableDatagram manager(driver, CLIENT_ADDRESS); // delivery/receipt manager
uint8_t messageBuf[RH_RF95_MAX_MESSAGE_LEN]; // buf for TX/RX handshake messages

// Sleep cycles
int sleepCycles; // track n of times we've slept. use to determine when to TX to base
const int cycleMatch = 3; // n  of sleep cycles (therefore n files gen'd) before we transmit
int lastFile; // track the last file we sent so we know where to start on next round

// File stuff
char* filenames[1440]; // array of pointers to hold filenames until transmit
int writeBufSize = (2 * numSensors) + 4; // size of the buffer for writing data to file
int readBufSize = (2 * numSensors) + 4; // size of the buffer for reading data off of file
int fileSize = (2 * numSensors) + 4; // size of the file for holding data

// Serial flash
const int FlashChipSelect = 4; // flash chip enable


/************ setup() ************/
void setup(void) {

  // init sleep cycles
  sleepCycles = 0;

  // init lastfile
  lastFile = 0;


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
  pinMode(13, OUTPUT);
//  digitalWrite(13, LOW);
  analogReadResolution(12);
  

  /************ USB monitor init ************/

  // Start serial communication
  Serial.begin(9600);
  while (!Serial); //Wait for serial comms


  /************ Temp sensors init ************/

  pinMode(tempPower, OUTPUT);
  digitalWrite(tempPower, HIGH);

  // init temp sensors
  sensors.begin();

  // for every sensor
  for (int i = 0; i < numSensors; i++) {

    // declare an array to hold the 8 byte address
    uint8_t addrHoldArray[8];

    // if error getting the address
    if (!sensors.getAddress(addr[i], i)) {

      // print error
      Serial.println("Couldn't find sensor at address 0");
      while (1);
    }

    // otherwise print address
    Serial.print("Sensor ");
    Serial.print(i, DEC);
    Serial.print(" online, address: ");
    printArr(addr[i], 8);
    Serial.println(" ");
  }

  // set sensors to 12 bit resolution
  sensors.setResolution(12);


  /************ Pinger init ************/

  // power it up
  pinMode(pingerPower, OUTPUT);
  digitalWrite(pingerPower, HIGH);

  // init serial comms
  pinger.begin(9600);

  // dump the first seven readings (all metadata)
  for (int i = 0; i < 7; i++) {
    pinger.readStringUntil('\r');
  }

  // get the a good reading, drop the leading 'R', convert int
  int pingerOutput = ((pinger.readStringUntil('\r')).substring(1)).toInt();

  Serial.print("Pinger online. Initial reading: ");
  Serial.println(pingerOutput, DEC);


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


  /************ Radio init ************/

  // if failure to init the manager
  if (!manager.init()) {

    // print error
    Serial.println("Unable to init radio");
    while (1);
  }
  // otherwise print success
  Serial.println("Radio online");

  // set parameters
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);


  /************ RTC init ************/

  // init the RTC
  rtc.begin();
  rtc.setTime(0, 0, 0);
  rtc.setDate(0, 0, 0);

  // RTC alarm setting on every 15 s resulting in 1 minute sleep period
  rtc.setAlarmSeconds(1);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(alarmMatch);

  // ***** IMPORTANT DELAY FOR CODE UPLOAD BEFORE USB PORT DETACH DURING SLEEP *****
  delay(15000);

  // put everything to sleep until alarm match
  SerialFlash.sleep();
  driver.sleep();
  oneWire.depower();
  pinger.end();
  digitalWrite(pingerPower, LOW);
  digitalWrite(tempPower, LOW);
  USBDevice.detach();
  rtc.standbyMode();
}


/************ loop() ************/
void loop() {


// pinger init
  // power up the probes
  digitalWrite(pingerPower, HIGH);
  digitalWrite(tempPower, HIGH);

  // wake up the pinger
  pinger.begin(9600);


//temp read
  // ask sensor to query temp
  sensors.requestTemperatures();

  // declare an array to hold the temperatures
  uint16_t tempArray[numSensors];

  // for every sensor
  for (int i = 0; i < numSensors; i++) {

    // get temp off of the sensor
    uint16_t temp = sensors.getTemp(addr[i]);

    // stuff it into the array
    tempArray[i] = temp;
  }

  // declare an array for writing data to file
  byte writeBuf[writeBufSize];

  int i = 0;
  while (i < 2 * numSensors) {

    // write temps to buffer as high and low byte
    writeBuf[i] = highByte(tempArray[i / 2]);
    writeBuf[i + 1] = lowByte(tempArray[i / 2]);

    // increment twice (two byte temp)
    i += 2;
  }


// pinger read
  // TODO: check that string matches 'R'[0-9][0-9][0-9][0-9]'\r' and if not discard it
  // TODO: Include a timer here to protect against errors. We don't want to get into
  // a situation where the pinger is just spitting back garbage (so Seral1.available() still
  // returns true) but this just hangs the microcontroller since it never spits out '\r'
  // and possibly runs us out of memory.

  // dump the first seven readings (all metadata)
  for (int j = 0; j < 7; j++) {
    pinger.readStringUntil('\r');
  }

  // get the a good reading
  int pingerOutput = ((pinger.readStringUntil('\r')).substring(1)).toInt();

  // write it to the buffer
  writeBuf[i] = highByte(pingerOutput);
  writeBuf[i+1] = lowByte(pingerOutput);


// battery read
  // get battery voltage (dump 1st read; innacurate)
  analogRead(A5);
  short adcRead = analogRead(A5);
  
  // write it to the buffer
  i+=2;
  writeBuf[i] = highByte(adcRead);
  writeBuf[i+1] = lowByte(adcRead);
  

// file write
  // get a timestamp filename
  char *filename = getFilename(rtc, filename);

  filenames[sleepCycles] = filename;
  
  // wake up the flash
  SerialFlash.wakeup();

  // create a file with that filename
  SerialFlash.create(filenames[sleepCycles], fileSize);
  
  // Open the file
  SerialFlashFile file = SerialFlash.open(filenames[sleepCycles]);
    
  // write the data to the file
  file.write((byte *)writeBuf, writeBufSize);

  // close the file
  file.close();

  // increment the cycle counter
  sleepCycles++;


// transmit
  // if we've done the appropriate number of cycles
  if ((sleepCycles % cycleMatch) == cycleMatch - 1) {

    byte readBuf[readBufSize];

    // for every file
    for (lastFile; lastFile < sleepCycles; lastFile++) {
      //      Serial.print("Sleep cycles: ");
      //      Serial.println(sleepCycles, DEC);
      //      Serial.print("lastFile: ");
      //      Serial.println(lastFile, DEC);

      // open the file
      file = SerialFlash.open(filenames[lastFile]);

      // read the temp data into a buffer
      file.read(readBuf, readBufSize);

      // create a buffer to hold the stuff we're going to send
      byte sendBuf[readBufSize + strlen(filenames[lastFile]) + 2];

      // read in size of the filename
      sendBuf[0] = (byte) strlen(filenames[lastFile]);

      // read the filename into the buffer as bytes
      int j;
      for (j = 1; j < strlen(filenames[lastFile]) + 1; j++) {
        sendBuf[j] = filenames[lastFile][j - 1];
        //        Serial.print((byte)filenames[lastFile][j - 1]);
      }
      
      // read in size of tempData
      //      j++;
      sendBuf[j] = (byte) sizeof(readBuf);

      // read the tempData in
      int k = j + 1;

      for (j = k; j < sizeof(sendBuf); j++) {
        sendBuf[j] = readBuf[j - k];
      }

      // if broadcast data successfully
      if (manager.sendtoWait(sendBuf, sizeof(sendBuf), SERVER_ADDRESS)) {
        
        // wait for a reply from the server
        uint8_t len = sizeof(messageBuf);
        uint8_t from;

        // if we receive reply
        if (manager.recvfromAckTimeout(messageBuf, &len, 2000, &from)) {

        }
        else
        {
          //          Serial.println("No reply, is rf95_reliable_datagram_server running?");
        }
      }
      else {
        //        Serial.println("Failed to broadcast data");
      }
    }

    // increment our last file counter
    lastFile++;
  }

  // otherwise sleep until next alarm match
  SerialFlash.sleep();
  driver.sleep();
  oneWire.depower();
  digitalWrite(pingerPower, LOW);
  digitalWrite(tempPower, LOW);
  USBDevice.detach();
  rtc.standbyMode();
}

// prints a simple char array
void printArr(uint8_t *ptr, int len) {
  for (int i = 0; i < len; i++) {
    Serial.print(ptr[i], HEX);
    Serial.print(" ");
  }
}

void alarmMatch() {
}
