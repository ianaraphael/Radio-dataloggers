/*

i2c_sensorController.ino

Sensor controller side code for I2C communication with SIMB

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

// collection state variable to keep track of what information we've sent to the
// simb. can have state 0 or 1.
// collectionState = 0
//    base state. simb has not resquested data. if we are in this state and get a
//    request, send the size of the data that we have to transmit and then switch
//    to collectionState = 1
// collectionState = 1
//    transmit state. we have already told the simb how much data we have to send.
//    now while still have data to transmit, send the next packet. when we are done,
//    return to base state
volatile int collectionState = 0;

// size of the data that we need to send to SIMB
volatile int n_dataToSend = 0;

char data[] = "Hello this is a really long message that won't fit into one packet. not sure why we're closing out.";

void setup() {

  // Begin serial comms
  Serial.begin(9600);

  // Initialize I2C comms as slave
  Wire.begin(SLAVE_ADDR);

  // Function to run when data requested from master
  Wire.onRequest(requestEvent);

}

void loop() {

  // if we're not in the middle of a transmission
  if (collectionState == 0) {

    // update the amount of data we have to send next time
    n_dataToSend = sizeof(data);
  }
}

void requestEvent() {

  // if we're in the base state
  if (collectionState == 0) {

    // tell the simb how much data we have to send
    Wire.write(n_dataToSend);

    Serial.print("SIMB requested data. We have ");
    Serial.print(n_dataToSend,DEC);
    Serial.println(" bytes to send.");

    // then set collection state variable to 1
    collectionState = 1;

    // if we're in the transmit state
  } else if (collectionState == 1) {

    int packetSize;

    // if we have more data than the max packet size
    if (n_dataToSend > MAX_PACKET_SIZE) {

      // send the max amount of data
      packetSize = MAX_PACKET_SIZE;

    } else { // otherwise

      // send the data we have left
      packetSize = n_dataToSend;

      // and set our state back to 0 since we'll be done sending data after this
      collectionState = 0;
    }

    // figure out where our current data starts and stops
    int startIndex = sizeof(data) - n_dataToSend;
    int endIndex = startIndex + packetSize - 1;

    // allocate an array for the current packet
    char currPacket[packetSize];

    // copy all of the data in
    for (int i=startIndex; i<=endIndex; i++){
      currPacket[i-startIndex] = data[i];
    }

    // now write the packet to the wire
    Wire.write(currPacket,sizeof(currPacket));

    // and update n_dataToSend
    n_dataToSend = sizeof(data) - endIndex - 1;

    Serial.print("Sending packet: ");
    Serial.println(currPacket);
    Serial.print(n_dataToSend,DEC);
    Serial.println(" bytes left to send.");
  }
}
