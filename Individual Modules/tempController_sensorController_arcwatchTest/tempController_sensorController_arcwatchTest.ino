/*

tempController_sensorController_arcwatchTest.ino

test script for to run i2c comms between simb temp controller and snowtatos
sensor controller board

Ian Raphael
ian.th@dartmouth.edu
2023.05.08
*/

// Include Arduino Wire library for I2C
#include <SnowTATOS_simbi2c.h>

void setup() {
  // Begin serial comms
  SerialUSB.begin(9600);

  init_I2C_simbSide(DATA_SIZE);

}


void loop() {

  // delay for 5 seconds
  delay(5000);

  // tell the sensor controller we're gonna ask for data
  alertSensorController();

  // delay for a few seconds to give it a change to collect
  delay(2000);

  // allocate a buffer to hold the data
  char returnData[DATA_SIZE];

  // get the data
  getDataFromSensorController(returnData);

  // finally, print the retrieved data to serial
  SerialUSB.println("");
  SerialUSB.print("Here's the data: ");
  SerialUSB.println(returnData);
  SerialUSB.println("");
}
