/*
   demonstrates concatenation of RTC data into a timestamp char
   array for use as a filename.


*/

#include <stdlib.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <RTCZero.h>

#define Serial SerialUSB

// Board constants
const int FlashChipSelect = 4;
const int LED = 13;

// Variables
unsigned char pinNumber;

const uint32_t filesize = 12;
char readBuf[filesize];

// Create rtc object
RTCZero rtc;


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

  // Set up serial flash
  SerialFlash.begin(FlashChipSelect);
  SerialFlash.wakeup();

  // RTC initialization
  rtc.begin();
  rtc.setTime(0, 0, 0);
  rtc.setDate(0, 0, 0);

  // RTC alarm setting on every 5 secs
  rtc.setAlarmSeconds(15);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(alarmMatch);

  Serial.println("Going into loop from setup");
}

void loop() {
  
  Serial.println("Going into sleep mode");
  USBDevice.detach();
  
  // standby the clock
  rtc.standbyMode();
}

void alarmMatch() {

  USBDevice.init();
  USBDevice.attach();
  Serial.println("Coming out of sleep mode");

  // declare array to hold timestamp
  char timeStamp[13];
  // make sure the array is empty
  timeStamp[0] = '\0';

  // concatenate the timestamp from RTC
  catStamp(timeStamp, rtc.getYear());
  catStamp(timeStamp, rtc.getMonth());
  catStamp(timeStamp, rtc.getDay());
  catStamp(timeStamp, rtc.getHours());
  catStamp(timeStamp, rtc.getMinutes());
  catStamp(timeStamp, rtc.getSeconds());

  // save timeStamp as filename
  const char* filename = timeStamp;

  // print the filename
  Serial.print("filename: ");
  Serial.println(filename);

  // start talking to monitor
  Serial.begin(9600);

  // wake up the chip
  SerialFlash.wakeup();

  // create a file object
  SerialFlashFile file;

  // open a file
  file = SerialFlash.open(filename);
  Serial.println("File opened successfully.");

  // close the file
  file.close();
  Serial.println("File closed.");

  // sleep the chip
  SerialFlash.sleep();
}


// function for concatenating time bits into char array holding ASCII version of
// the integers
void catStamp(char* timeStamp, uint8_t intTime) {

  // allocate array to hold 2 digits + null char of time bit
  char charTimeBuf[3];

  // convert uint8_t to char, store in buffer
  itoa(intTime, charTimeBuf, 10);

  // if it's a one digit number
  if (strlen(charTimeBuf) == 1) {

    // shift null terminating character right
    charTimeBuf[2] = '\0';
    // shift original digit right
    charTimeBuf[1] = charTimeBuf[0];
    // pad with a 0
    charTimeBuf[0] = '0';
  }

  // append the time bit to the timeStamp
  strcat(timeStamp, charTimeBuf);

  return;
}
