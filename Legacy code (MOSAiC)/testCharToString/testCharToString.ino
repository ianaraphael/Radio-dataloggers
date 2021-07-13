
#include <RTCZero.h>

#define Serial SerialUSB

// Create rtc object
RTCZero rtc;

// 
char buf[6];

void setup() {
 
  // RTC initialization
 rtc.begin();
 rtc.setTime(0, 0, 0);
 rtc.setDate(0, 0, 0);
}

void loop() {
 buf[0] = rtc.getYear();
 buf[1] = rtc.getMonth();
 buf[2] = rtc.getDay();
 buf[3] = rtc.getHours();
 buf[4] = rtc.getMinutes();
 buf[5] = rtc.getSeconds();

 Serial.println(String(buf[0], DEC));
 Serial.println(charToString(buf));
 delay(1000);
}

String charToString(char* charArr) {
  String outString = padString(charArr[0]);
  for (int i=1; i<6; i++) {
    outString = outString + padString(charArr[i]);//String(charArr[i], DEC);
  }
  return outString;
}

String padString(char val) {
  String outString = String(val, DEC);
  if (outString.length()==1) {
    outString = String('0') + outString;
  }
  return outString;
}
