#include <timestamp.h>
#include <stdlib.h>
#include <RTCZero.h>

// Create rtc object
RTCZero rtc;

unsigned char pinNumber;

#define Serial SerialUSB

void setup() {


  // Switch unused pins as input and enabled built-in pullup
  for (pinNumber = 0; pinNumber < 23; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }

  for (pinNumber = 32; pinNumber < 42; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }

  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);

  // RTC initialization
  rtc.begin();
  rtc.setTime(0, 0, 0);
  rtc.setDate(0, 0, 0);

  // RTC alarm setting on every 5 secs
  rtc.setAlarmSeconds(15);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(alarmMatch);
}

void loop() {

  // get the filename
  char* fileName = getFilename(rtc, fileName);

  // print the filename
  Serial.println(fileName);

  // !!don't!! forget to free the memory
  free(fileName);
  
  delay(3000);
}


void alarmMatch() {
}
