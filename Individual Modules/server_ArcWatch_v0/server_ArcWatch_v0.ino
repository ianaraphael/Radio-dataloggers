/*

server_arcwatch_v0.ino

Server code for ArcWatch SnowTATOS

Ian Raphael
ian.th@dartmouth.edu
2023.05.02
*/

// collection state variable to keep track of what information we've sent to the
// simb.
// i2cCollectionState = -1
//    sleep state. SC is in deep sleep mode and not ready to transmit data
// i2cCollectionState = 0
//    pre-sleep state. SC is ready to go back to sleep after this
// i2cCollectionState = 1
//    metadata state. simb asked for data size. we'll package the size of the
//    data that we have to send, then move to state 2 (standby)
// i2cCollectionState = 2
//    standby state. wait for a data reqest interrupt from the SIMB, then put the data buffer on the wire
// i2cCollectionState = 3
//    data state. simb is asking for the actual data. we'll package the next dataframe,
//    then go to standby state.
volatile int i2cCollectionState = -1;

#define STATION_ID SERVER_ADDRESS // our station id is the server address (0)

#include <SnowTATOS_server.h>

// size of the data that we need to send to SIMB
volatile uint16_t n_bytesLeftToSendSIMB = 0;

volatile uint16_t currI2cPacketSize;

// indicates whether we're with a client and which client it is. default to server_address when on standby.
int volatile withClient = SERVER_ADDRESS;

// declare a global file object to write stuff onto
File dataFile;

// declare a global filename
String filename;

/****** setup ******/
void setup() {
  // Wait for serial port to be available
  // while (!SerialUSB);

  // start serial comms with the computer
  SerialUSB.begin(9600);

  // init the radio
  init_Radio();

  // init SD
  init_SD();

  init_RTC();

  // print success
  SerialUSB.println("Server init success");
}


void loop() {

  // check if we're dealing with the SIMB right now. if we are, bypass radio comms
  // until we're done with SIMB.
  if (!withSimb) {

    // ********** radio comms with clients ********** //

    // allocate a buffer with max packet size
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

    // allocate same sized buffer but with char
    char msg[sizeof(buf)];

    // wipe it clean
    buf[0] = '\0';

    // if the manager isn't busy right now
    if (manager.available()) {

      // copy the length of the buffer
      uint8_t len = sizeof(buf);
      // allocate a char to store station id
      uint8_t from;

      // if we've gotten a message, receive and store it, its length, and station id
      if (manager.recvfromAck(buf, &len, &from)) {

        // if the message is from a client we're already dealing with *or* if we're not with a client at all (standby mode)
        // otherwise we're just going to ignore the message
        if (from == withClient || withClient == SERVER_ADDRESS) {

          // add safeguard to exit client service mode after timeout, even without recieving termination message

          // get the first char
          uint8_t messageType = buf[0];

          // and switch on the message contents
          switch (messageType) {

            // if it's a transaction begin/end message
            case 0:

            // if it's from a client we're already dealing with
            if (withClient == from) {

              // then it's a transaction termination message;
              // switch withClient variable back to server address to indicate "standby" mode
              withClient = SERVER_ADDRESS;

              SerialUSB.print("transaction with client #");
              SerialUSB.print(from, DEC);
              SerialUSB.println(" terminated.");
              SerialUSB.println("----------");
              SerialUSB.println("");

              // if we're in standby mode
            } else if (withClient == SERVER_ADDRESS) {

              // it's a transaction initiation message; change withClient to remember which client we're talking to
              // TODO: add an alarmed timeout here so that we're not stuck dealing with a client forever
              withClient = from;

              // get the filename
              filename = "";
              filename = String((char*)&buf[1]);

              SerialUSB.print("Initiating transaction with client #");
              SerialUSB.println(from, DEC);
              SerialUSB.print("Creating/opening file: ");
              SerialUSB.println(filename);

              // init the SD card
              init_SD();

              // get the file size on our end
              unsigned long numBytesServer = getFileSize(filename);

              // and send it back as a handshake to the client
              SerialUSB.print("Sending handshake back to client. File size: ");
              SerialUSB.println(numBytesServer);
              // if the client fails to acknowledge handshake
              if (!manager.sendtoWait((uint8_t *) &numBytesServer, sizeof(&numBytesServer), from)) {
                SerialUSB.println("Client failed to acknowledge reply");
                // return to standby mode
                withClient = SERVER_ADDRESS;
              }
            }
            break;

            // case 1 means it's data
            case 1:

            // if we're in a transaction with this client
            if (withClient == from) {

              // print the data out
              SerialUSB.print((char*) &buf[1]);

              // write the data to the file
              writeToFile(filename,String((char*) &buf[1]));

              break;

              // otherwise our transaction with this client has timed out and they
              // will have to try again later. ignore this message.
            } else if (withClient == SERVER_ADDRESS) {
              break;
            }
          }
        }
      }
    }
  }


  // ********** I2C with SIMB ********** //


  // switch on the i2cCollectionState variable
  switch(i2cCollectionState) {

    // sleep
    // sleep state. either just coming out of sleep or waiting to go back to sleep.
    // do nothing.
    case -1:
      break;

    // metadata
    // simb has asked for data. get the data size, pack the first packet, switch to standby state
    case 1:

      // update the amount of data we have to send
      n_bytesLeftToSendSIMB = sizeof(data);

      SerialUSB.print("SIMB requested data size. We have ");
      SerialUSB.print(n_bytesLeftToSendSIMB,DEC);
      SerialUSB.println(" bytes to send. Packaging value and standing by.");

      // put the value into the send buffer
      i2cSendBuf[0] = highByte(n_bytesLeftToSendSIMB);
      i2cSendBuf[1] = lowByte(n_bytesLeftToSendSIMB);

      // define the packet size
      currI2cPacketSize = sizeof(uint16_t);

      // switch to i2cCollectionState 2 (standby)
      i2cCollectionState = 2;

      break;

    // standby
    // waiting for a data request from simb
    case 2:
      break; // do nothing. handled in ISR.

    // transmit
    // simb has requested data. package the data to send back
    case 3:

      // if we have more data than the max packet size
      if (n_bytesLeftToSendSIMB > MAX_PACKET_SIZE) {

        // send the max amount of data
        currI2cPacketSize = MAX_PACKET_SIZE;

      } else { // otherwise

        // send the data we have left
        currI2cPacketSize = n_bytesLeftToSendSIMB;

        // set collection state to 0 (pre-sleep) since we'll be done sending data after this
        i2cCollectionState = 0;
      }

      // figure out where our current data starts and stops
      int startIndex = sizeof(data) - n_bytesLeftToSendSIMB;
      int endIndex = startIndex + currI2cPacketSize - 1;

      // copy all of the data in
      for (int i=startIndex; i<=endIndex; i++){
        i2cSendBuf[i-startIndex] = data[i];
      }

      // and update n_bytesLeftToSendSIMB
      n_bytesLeftToSendSIMB = sizeof(data) - endIndex - 1;

      // add a null terminator to the buffer
      i2cSendBuf[currI2cPacketSize] = '\0';

      // preview print
      SerialUSB.print("SIMB has requested data. Packaging: ");
      SerialUSB.println((char*) i2cSendBuf);

      // print how much we have left to send
      SerialUSB.print(n_bytesLeftToSendSIMB,DEC);
      SerialUSB.println(" bytes left to send.");

      // if we're not supposed to go to sleep
      if (i2cCollectionState == 3) {
        // go back into standby
        i2cCollectionState = 2;
      }
      break;
  }

  // if we're done with this transaction
  if (i2cCollectionState == -1) {

    // TODO: clear the interrupt register

    // reattach our chip select interrupt
    attachInterrupt(digitalPinToInterrupt(SENSORCONTROLLER_CS), simbInterruptHandler, LOW);

    // TODO: go into deep sleep
  }
}
