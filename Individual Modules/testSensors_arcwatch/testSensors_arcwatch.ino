/*

script for testing the temp and snow pinger sensors on arcwatch

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.08.26

*/

#include "SnowTATOS.h"

#define Serial SerialUSB // comment if not using rocketscream boards

void setup() {

  boardSetup();

  // Begin serial comms
  Serial.begin(9600);
  delay(5000);

  // init the temperature sensors
  initTemps();
}



void loop() {

  // read the data
  uint8_t pingerData = readPinger();
  static float tempData[NUM_TEMP_SENSORS];
  readTemps(tempData);

  // print out the temp data
  Serial.println("temp data: ");
  for (int i=0;i<NUM_TEMP_SENSORS;i++) {
    Serial.println(tempData[i],3);
  }
  Serial.println("pinger data: ");
  Serial.println(pingerData);
}
