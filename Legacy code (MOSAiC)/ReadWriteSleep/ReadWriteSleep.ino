/* ReadWriteSleep
 *  Demonstrates the board's ability to read analog
 *  data, write that data to serial flash, and
 *  use the rtc to sleep and wake up 
 */

#include <SerialFlash.h>
#include <SPI.h>
#include <RTCZero.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#define Serial SerialUSB

// Board constants
const int FlashChipSelect = 4;
const int RadioDio0 = 2;
const int RadioChipSelect = 5;
const int LED = 13;
const int RF95_FREQ = 915;
const int ClientAddress = 1;
const int ServerAddress = 2;

// Create radio and class to manage deliver and receipt
RH_RF95 driver(RadioChipSelect, RadioDio0);
RHReliableDatagram manager(driver, ClientAddress);

// Create rtc object
RTCZero rtc;

// Parameters
const int NumberOfMeasurements = 5;

// Variables
unsigned short adcReading;
unsigned char pinNumber;
char buf[6 + 2*NumberOfMeasurements];
const char *filename = "test.bin";
const uint32_t filesize = 160;
bool fileCreated = false;
char readBuf[filesize];


void setup() {
 // In case we want to upload some different code
 USBDevice.init();
 USBDevice.attach();
 
 // Switch unused pins as input and enabled built-in pullup 
 for (pinNumber = 0; pinNumber < 23; pinNumber++) {
  pinMode(pinNumber, INPUT_PULLUP);
 }

 for (pinNumber = 32; pinNumber < 42; pinNumber++) {
  pinMode(pinNumber, INPUT_PULLUP);
 }

 pinMode(25, INPUT_PULLUP);
 pinMode(26, INPUT_PULLUP);

 if (!driver.init()){}
 driver.sleep();

 // Set up serial flash
 SerialFlash.begin(FlashChipSelect);
 SerialFlash.wakeup();
 // If the file doesn't already exist create it
 if (!SerialFlash.exists(filename)) {
  digitalWrite(LED, HIGH);
  SerialFlash.create(filename, filesize);
  delay(10);
  digitalWrite(LED, LOW);
  fileCreated = true;
 }

 // If the serial monitor is connected, switch into alternate mode where we
 // display the data currently on the chip
 Serial.begin(9600);
 delay(15000);
 if (Serial) {
  Serial.println("Serial comms online, switching to data dump mode");
  SerialFlashFile file;
  file = SerialFlash.open(filename);
  Serial.print("Filesize: ");
  Serial.println(file.size());
  Serial.print("Available: ");
  Serial.println(file.available());
  Serial.print("Was the file created this startup? ");
  Serial.println(fileCreated);

  file.read(readBuf, filesize);
  for (int i=0; i<filesize; i++) {
    Serial.print((int)readBuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
//  char buf2[2];
//  buf2[0] = 1;
//  buf2[1] = 2;
//  file.write(buf2, 2);
  
  file.close();
  Serial.println("Done dumping data, returning to datalogging");
 }
 
 SerialFlash.sleep();

 pinMode(LED_BUILTIN, OUTPUT);
 
 digitalWrite(LED_BUILTIN, HIGH);

 // ***** IMPORTANT DELAY FOR CODE UPLOAD BEFORE USB PORT DETACH DURING SLEEP *****
 delay(15000);

 //***** UNCOMMENT FOR MINI ULTRA PRO VERSION 0V30 & ABOVE IF PINS ARE CONNECTED TO RADIO ****
 /*pinMode(6, INPUT);
 pinMode(7, INPUT);
 digitalWrite(3, HIGH);
 pinMode(3, OUTPUT);
 digitalWrite(3, HIGH);*/
 //*****

 // RTC initialization
 rtc.begin();
 rtc.setTime(0, 0, 0);
 rtc.setDate(0, 0, 0);

 // RTC alarm setting on every 15 s resulting in 1 minute sleep period
 rtc.setAlarmSeconds(1);
 rtc.enableAlarm(rtc.MATCH_SS);
 rtc.attachInterrupt(alarmMatch);

 // Set the analog resolution to use the full 12 bits
 analogReadResolution(12);
 
 digitalWrite(LED_BUILTIN, LOW);
 
 USBDevice.detach();
 rtc.standbyMode();
}

void loop() {
 // toggle led
 //digitalWrite(LED_BUILTIN, HIGH);

 // write the rtc values to the buffer
 buf[0] = rtc.getYear();
 buf[1] = rtc.getMonth();
 buf[2] = rtc.getDay();
 buf[3] = rtc.getHours();
 buf[4] = rtc.getMinutes();
 buf[5] = rtc.getSeconds();

 // Discard inaccurate first adc reading
 adcReading = analogRead(A5);
 
 // read the battery NumberOfMeasurements Times:
 for (int i = 0; i<NumberOfMeasurements; i++) {
  adcReading = analogRead(A5);
  buf[6 + 2*i] = highByte(adcReading);
  buf[6 + 2*i + 1] = lowByte(adcReading);
 }

 // Wake up the serial flash and append to our file
 SerialFlash.wakeup();
 SerialFlashFile file2;
 file2 = SerialFlash.open(filename);
 // If there is space available write buffer to the file
 if (file2.available() >= (uint32_t)sizeof(buf)) {
  digitalWrite(LED_BUILTIN, HIGH);
  file2.write(buf, (uint32_t)sizeof(buf));
  digitalWrite(LED_BUILTIN, LOW);
 }
 // Close file and sleep serial
 file2.close();
 SerialFlash.sleep();

 // Toggle led
 //digitalWrite(LED_BUILTIN, LOW);
 // Sleep until next alarm match
 rtc.standbyMode();
}

void alarmMatch() {
}
