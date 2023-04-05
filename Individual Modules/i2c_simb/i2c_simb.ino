/*

i2c_simb.ino

SIMB3 side code for I2C communication with Ian's sensor controller

Ian Raphael
ian.th@dartmouth.edu
2023.03.13
*/

// Include Arduino Wire library for I2C
#include <Wire.h>

#define SLAVE_ADDR 9 // sensor controller (SC) I2C Address
#define MAX_PACKET_SIZE 32 // maximum dataframe size
#define SC_CS 6 // sensor controller chip select
#define CS_DELAY 10 // number of milliseconds to wait with chip select pin low

void setup() {
  // Begin serial comms
  SerialUSB.begin(9600);

  // Initialize I2C communications as Master
  Wire.begin();

  // set pin mode for sensor controller chip select and write it high (active low)
  pinMode(SC_CS,OUTPUT);
  digitalWrite(SC_CS,HIGH);


}


void loop() {

  // delay for 5 seconds
  delay(5000);

  // write sc chip select low (active) to wake up SC and say we're going to ask for data
  digitalWrite(SC_CS,LOW);
  // delay for a moment
  delay(CS_DELAY);
  // then write high again
  digitalWrite(SC_CS,HIGH);

  SerialUSB.println("Requesting data from sensor controller");

  // flush the wire before we start reading anything
  while(Wire.available()){
    Wire.read();
  }

  // allocate an array to hold the data size indicator. we'll cast this to an int later
  uint8_t byteArray[2];

  // ask SC to tell us how much data it has to send over
  Wire.requestFrom(SLAVE_ADDR, sizeof(uint16_t));

  // read the data off the line
  uint8_t b1 = (uint8_t) Wire.read();
  uint8_t b2 = (uint8_t) Wire.read();

  // recast to an int
  uint16_t n_dataToGet = ((uint16_t) b1<<8)|b2;

  // print to serial
  SerialUSB.print("SC has ");
  SerialUSB.print(n_dataToGet,DEC);
  SerialUSB.println(" bytes to send.");

  // Allocate an array to hold the data
  char returnData[n_dataToGet];

  // counter variable to track where we are in the total data retrieval session
  int i_session = 0;

  // while we haven't gotten all of the data
  while (n_dataToGet > 0) {

    // declare variable to keep track of the packet size we're asking for
    int packetSize;

    // if what's left is more than the maximum packet size
    if (n_dataToGet > MAX_PACKET_SIZE) {
      // request the maximum packet size
      packetSize = MAX_PACKET_SIZE;
    } else { // otherwise
      // get what's left
      packetSize = n_dataToGet;
    }

    SerialUSB.print("We expect this many bytes from SC: ");
    SerialUSB.println(packetSize,DEC);

    // ask the SC to put the data on the wire
    SerialUSB.print("SC put this many bytes on the wire: ");
    int nBytesCurrPacket = Wire.requestFrom(SLAVE_ADDR,packetSize);
    SerialUSB.println(nBytesCurrPacket,DEC);

    // counter variable to track where we are in the current data packet
    int i_currPacket = 0;

    // then, while we haven't gotten read all of this packet
    while (i_currPacket<packetSize) {

      // read the next byte off the wire into the array
      returnData[i_session] = Wire.read();

      // and increment the counters
      i_session++;
      i_currPacket++;
    }

    // now decrement n_dataToGet by the amount that we read
    n_dataToGet = n_dataToGet - packetSize;
  }

  // finally, print the retrieved data to serial
  SerialUSB.println("");
  SerialUSB.print("Here's the data: ");
  SerialUSB.println(returnData);
  SerialUSB.println("");
}
