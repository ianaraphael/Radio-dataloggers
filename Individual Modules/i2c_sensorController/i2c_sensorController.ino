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
#define SC_CS 6 // chip select

// collection state variable to keep track of what information we've sent to the
// simb.
// collectionState = -1
//    sleep state. SC is in deep sleep mode and not ready to transmit data
// collectionState = 0
//    pre-sleep state. SC is ready to go back to sleep after this
// collectionState = 1
//    metadata state. simb asked for data size. we'll package the size of the
//    data that we have to send, then move to state 2 (standby)
// collectionState = 2
//    standby state. wait for a data reqest interrupt from the SIMB, then put the data buffer on the wire
// collectionState = 3
//    data state. simb is asking for the actual data. we'll package the next dataframe,
//    then go to standby state.
volatile int collectionState = -1;

// size of the data that we need to send to SIMB
volatile uint16_t n_dataToSend = 0;
volatile uint8_t sendBuf[MAX_PACKET_SIZE+1]; // add one byte for a null terminator so that we can print on this side. not for transmission.
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

  // pull the chip select pin down
  pinMode(SC_CS,INPUT_PULLDOWN);

  SerialUSB.println("Sensor controller initiated, waiting for SIMB to initialize");

  // wait around until SIMB gives us the go ahead by shifting the chip select high
  while(digitalRead(SC_CS) == LOW) {
  }

  SerialUSB.println("SIMB activated, attaching interrupt");

  // pull up the chip select pin
  pinMode(SC_CS,INPUT_PULLUP);

  // delay for a moment
  delay(10);

  // then attach an interrupt to trigger on a low pin
  attachInterrupt(digitalPinToInterrupt(SC_CS), simbInterruptHandler, LOW);
}




void loop() {

  // switch on the collectionState variable
  switch(collectionState) {

    // sleep
    // sleep state. either just coming out of sleep or waiting to go back to sleep.
    // do nothing.
    case -1:
      break;

    // metadata
    // simb has asked for data size. package the data size and switch to standby state
    case 1:


      // update the amount of data we have to send
      n_dataToSend = sizeof(data);

      SerialUSB.print("SIMB requested data size. We have ");
      SerialUSB.print(n_dataToSend,DEC);
      SerialUSB.println(" bytes to send. Packaging value and standing by.");

      // put the value into the send buffer
      sendBuf[0] = highByte(n_dataToSend);
      sendBuf[1] = lowByte(n_dataToSend);

      // define the packet size
      packetSize = sizeof(uint16_t);

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

      // if we have more data than the max packet size
      if (n_dataToSend > MAX_PACKET_SIZE) {

        // send the max amount of data
        packetSize = MAX_PACKET_SIZE;

      } else { // otherwise

        // send the data we have left
        packetSize = n_dataToSend;

        // set collection state to 0 (pre-sleep) since we'll be done sending data after this
        collectionState = 0;
      }

      // figure out where our current data starts and stops
      int startIndex = sizeof(data) - n_dataToSend;
      int endIndex = startIndex + packetSize - 1;

      // copy all of the data in
      for (int i=startIndex; i<=endIndex; i++){
        sendBuf[i-startIndex] = data[i];
      }

      // and update n_dataToSend
      n_dataToSend = sizeof(data) - endIndex - 1;

      // add a null terminator to the buffer
      sendBuf[packetSize] = '\0';

      // preview print
      SerialUSB.print("SIMB has requested data. Packaging: ");
      SerialUSB.println((char*) sendBuf);

      // print how much we have left to send
      SerialUSB.print(n_dataToSend,DEC);
      SerialUSB.println(" bytes left to send.");

      // if we're not supposed to go to sleep
      if (collectionState == 3) {
        // go back into standby
        collectionState = 2;
      }
      break;
  }

  // if we're supposed to go to sleep
  if (collectionState == -1) {

    // TODO: clear the interrupt register

    // reattach our chip select interrupt
    attachInterrupt(digitalPinToInterrupt(SC_CS), simbInterruptHandler, LOW);

    // TODO: go into deep sleep

  }
}


// service routine for getting a Wire data request from the SIMB. Sends all the data back
void requestEvent() {

  // write the data to the line
  Wire.write((uint8_t *)sendBuf,(uint16_t)packetSize);

  // if we're supposed to go to sleep after this
  if (collectionState == 0) {

    // enter sleep state
    collectionState = -1;

  } else if (collectionState == 2) {

    // go to data packaging state
    collectionState = 3;
  }
}


// service routine for handling chip select interrupt from SIMB
void simbInterruptHandler(void) {

  // if chip select is actually low
  if (digitalRead(SC_CS) == LOW) {

    // detach the interrupt so we don't get stuck in a loop
    detachInterrupt(digitalPinToInterrupt(SC_CS));

    // and set collection state variable to 1 (package metadata)
    collectionState = 1;
  }
}
