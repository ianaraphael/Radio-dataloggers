/*

tempController_sensorController_arcwatchTest.ino

test script to run i2c comms between simb temp controller and snowtatos
sensor controller board

Ian Raphael
ian.th@dartmouth.edu
2023.05.08
*/

// Include Arduino Wire library for I2C
#include "SnowTATOS.h"
#include "SnowTATOS_simbi2c.h"

#define Serial SerialUSB // comment if not using rocketscream boards

void setup() {
  // Begin serial comms
  Serial.begin(9600);

  init_sensorController_simbSide();

}


void loop() {

  // delay for 30 seconds
  delay(30000);

  // tell the sensor controller we're gonna ask for data
  alertSensorController();

  // delay for a few seconds to give it a chance to collect
  delay(3000);

  // allocate a buffer to hold the data
  uint8_t returnData[SIMB_DATASIZE];
  maskSimbData(returnData);

  // get the data
  getDataFromSensorController(returnData);

  // print the data in binary
  for (int i=0;i<SIMB_DATASIZE;i++){
    SerialUSB.print(returnData[i],HEX);
    SerialUSB.print(" ");
  }


  // now print the data for all ten stations
  for (int stn=1;stn<=NUM_STATIONS;stn++) {

    Serial.print("Station #");
    Serial.println(stn,DEC);
    Serial.println("");

    Serial.println("Temperatures: ");

    // allocate an array to hold the temps
    float tempArray[3];
    // unpack them
    unpackTempData((uint8_t*)returnData,tempArray,stn);

    // and print them
    for (int i2 = 0;i2<NUM_TEMP_SENSORS;i2++){
      Serial.println(tempArray[i2],4);
    }
    Serial.println("");

    Serial.println("Pinger:");
    Serial.println(unpackPingerData((byte*)returnData,stn),DEC);

    Serial.println("__________________________");
  }
}
