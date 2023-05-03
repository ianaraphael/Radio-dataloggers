/*

server_arcwatch_v0.ino

Server code for ArcWatch SnowTATOS

Ian Raphael
ian.th@dartmouth.edu
2023.05.02
*/

#define STATION_ID SERVER_ADDRESS // our station id is the server address (0)

#include <SnowTATOS.h>

// indicates whether we're with a client and which client it is. default to server_address when on standby.
int volatile withClient = SERVER_ADDRESS;
int withSIMB = 999; // indicates that we're in transaction with SIMB

// declare a global file object to write stuff onto
File dataFile;

// declare a global filename
String filename;

/****** setup ******/
void setup()
{
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
