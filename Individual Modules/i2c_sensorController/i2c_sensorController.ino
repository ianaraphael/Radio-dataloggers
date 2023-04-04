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
#define SC_CS 2 // chip select



// collection state variable to keep track of what information we've sent to the
// simb.
// collectionState = -1
//    sleep state. simb is in deep sleep mode and not ready to transmit data
// collectionState = 0
//    standby state. simb has woken sensor controller using the chip sleect,
//    but hasn't requested data info yet. waiting for a wire request, then
//    switching to state 1
// collectionState = 1
//    metadata state. simb asked for data size. we'll return the size of the
//    data that we have to send, then move to state 2 (standby)
// collectionState = 2
//    standby state.
// collectionState = 3
//    simb is asking for data. we'll send the next dataframe. if we're done, we'll
//    go to "sleep" (-1). otherwise we'll go back to standby (2)
volatile int collectionState = -1;

// size of the data that we need to send to SIMB
volatile uint16_t n_dataToSend = 0;
volatile uint8_t sendBuf[MAX_PACKET_SIZE];
volatile uint16_t packetSize;

char data[] = "Hello this is a really long message that won't fit into one packet.";

void setup() {

  // Begin serial comms
  SerialUSB.begin(9600);

  delay(5000);

  // Initialize I2C comms as slave
  Wire.begin(SLAVE_ADDR);

  // Function to run when data requested from master
  Wire.onRequest(requestEvent);

  // pull up the chip select pin
  pinMode(SC_CS,INPUT_PULLUP);

  SerialUSB.println("Im here i promise");

  // attach an interrupt to trigger on a low pin
  attachInterrupt(digitalPinToInterrupt(SC_CS), simbInterruptHandler, LOW);
}




void loop() {

  // switch on the collectionState variable
  switch(collectionState) {

    // sleep
    // emulates sleep state. should not evaluate once sleep is enabled.
    case -1:
      break;

    // base
    // just out of sleep, no data request yet.
    case 0:
      break; // do nothing. handled in ISR.

    // metadata
    // simb has asked for data size. package the data size and switch to standby state
    case 1:

      SerialUSB.print("SIMB requested data. We have ");
      SerialUSB.print(n_dataToSend,DEC);
      SerialUSB.println(" bytes to send.");

      // put the value into the send buffer
      sendBuf[0] = highByte(n_dataToSend);
      sendBuf[1] = lowByte(n_dataToSend);

      packetSize = sizeof(uint16_t);

      // SerialUSB.println("Sending these bytes: ");
      // SerialUSB.println(sendBuf[0],HEX);
      // SerialUSB.println(sendBuf[1],HEX);
      //
      // SerialUSB.print("With packet size: ");
      // SerialUSB.println(packetSize,DEC);


      // tell the simb how much data we have to send
      // Wire.write(byteArray,sizeof(byteArray));

      // switch to collectionState 2 (standby)
      collectionState = 2;

      break;

    // standby
    // waiting for a data request from simb
    case 2:
      break; // do nothing. handled in ISR.

    // transmit
    // simb has requested data. package the data to send back
    case 3:

      // declare variable for packet size
      // int packetSize;

      // if we have more data than the max packet size
      if (n_dataToSend > MAX_PACKET_SIZE) {

        // send the max amount of data
        packetSize = MAX_PACKET_SIZE;

      } else { // otherwise

        // send the data we have left
        packetSize = n_dataToSend;

        // set collection state to -1 (sleep) since we'll be done sending data after this
        collectionState = -1;
      }

      // figure out where our current data starts and stops
      int startIndex = sizeof(data) - n_dataToSend;
      int endIndex = startIndex + packetSize - 1;

      // allocate an array for the current packet
      char currPacket[packetSize];

      // copy all of the data in
      // for (int i=startIndex; i<=endIndex; i++){
      //   currPacket[i-startIndex] = data[i];
      // }

      // copy all of the data in
      for (int i=startIndex; i<=endIndex; i++){
        sendBuf[i-startIndex] = data[i];
      }

      // now write the packet to the wire
      // Wire.write(currPacket,sizeof(currPacket));

      // and update n_dataToSend
      n_dataToSend = sizeof(data) - endIndex - 1;
      // n_dataToSend = sizeof(data) - endIndex;

      SerialUSB.print("Sending packet: ");
      SerialUSB.println((char*) sendBuf);
      SerialUSB.print(n_dataToSend,DEC);
      SerialUSB.println(" bytes left to send.");

      // if we're not supposed to go to sleep
      if (collectionState == 3) {
        // go back into standby
        collectionState = 2;
      }
      break;
  }
}


// service routine for getting a Wire data request from the SIMB. Sends all the data back
void requestEvent() {

  // write the data to the line
  Wire.write((uint8_t *)sendBuf,(uint16_t)packetSize);

  // reattach our chip select interrupt
  attachInterrupt(digitalPinToInterrupt(SC_CS), simbInterruptHandler, LOW);

  // if we're supposed to go to sleep
  if (collectionState == -1) {
    // TODO: enable deep sleep
  }
}


// service routine for handling chip select interrupt from SIMB
void simbInterruptHandler(void) {

  // detach the interrupt so we don't get stuck in a loop
  detachInterrupt(digitalPinToInterrupt(SC_CS));

  // then, if we were asleep
  if (collectionState == -1) {

    // update the amount of data we have to send
    n_dataToSend = sizeof(data);

    // and set collection state variable to 1
    collectionState = 1;

  } else if (collectionState == 2) {

    // then set collection state variable to 3 (transmit state)
    collectionState = 3;
  }

}
