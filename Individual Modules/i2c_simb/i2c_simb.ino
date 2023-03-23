/*

i2c_simb.ino

SIMB3 side code for I2C communication with Ian's sensor controller

Ian Raphael
ian.th@dartmouth.edu
2023.03.13
*/

// Include Arduino Wire library for I2C
#include <Wire.h>

// Define sensor controller (SC) I2C Address
#define SLAVE_ADDR 9
#define MAX_PACKET_SIZE 32

// for Rocketscream board. Not necessary if using Arduino board
#define Serial SerialUSB

void setup() {

  // Initialize I2C communications as Master
  Wire.begin();

  // Begin serial comms
  Serial.begin(9600);
}


void loop() {

  // delay for 5 seconds
  delay(5000);

  Serial.println("Requesting data from sensor controller");

  // declare a data size indicator as 0
  int n_dataToGet = 0;

  // ask SC how much data it has to send over
  Wire.requestFrom(SLAVE_ADDR, sizeof(int));
  n_dataToGet = (int) Wire.read();

  Serial.print("SC has ");
  Serial.print(n_dataToGet,DEC);
  Serial.println(" bytes to send.");

  // Allocate an array to hold the data
  char returnData[n_dataToGet];

  // counter var
  int i = 0;

  // while we haven't gotten all of the data
  while (n_dataToGet > 0) {

    int packetSize;

    // if what's left is more than the maximum packet size
    if (n_dataToGet > MAX_PACKET_SIZE) {
      // request the maximum packet size
      packetSize = MAX_PACKET_SIZE;
    } else { // otherwise
      // get what's left
      packetSize = n_dataToGet;
    }

    // ask the SC for data
    Wire.requestFrom(SLAVE_ADDR,packetSize);

    int i2 = 0;
    // then, while we haven't gotten all of the data
    while (i2<packetSize) {

      // read the next byte into the array
      returnData[i] = Wire.read();

      // and increment the counters
      i++;
      i2++;
    }

    // now decrement n_dataToGet by the amount that we read
    n_dataToGet = n_dataToGet - packetSize;

  }

  // Now print the data to serial
  Serial.println(returnData);
}
