/*
 * timestamp.cpp
 *
 * Library functions for getting a filename char array built out of RTC
 * timestamp bits
 */

#include <stdlib.h>
#include <RTCZero.h>
#include "timestamp.h"

#define Serial SerialUSB

// concatenates time bits into char array holding as ASCII chars of input ints
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
}

// reads RTC when called, returns timestamp filename
char* getFilename(RTCZero rtc, char* timeStamp) {

  timeStamp = (char*)malloc(13);

  // make sure the array is empty
  timeStamp[0] = '\0';

  // concatenate the timestamp from RTC
  catStamp(timeStamp, rtc.getYear());
  catStamp(timeStamp, rtc.getMonth());
  catStamp(timeStamp, rtc.getDay());
  catStamp(timeStamp, rtc.getHours());
  catStamp(timeStamp, rtc.getMinutes());
  catStamp(timeStamp, rtc.getSeconds());

  return timeStamp;
}
