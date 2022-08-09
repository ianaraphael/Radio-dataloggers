//**** server pseudocode ****//

// Sit around and wait for a message (open mode)
// If I'm in open mode and receive a message _immediately_ go into closed mode, and save the sender ID
// check the message type. if it's a handshake message
// get and save the filenames that the client wants to transmit
// check the number of lines of each that we have locally
// send a message back with those numbers
// wait for a message back
// when I receive a message
// if it's from the client I'm already talking to and it's a data message
// append the data to the appropriate local files
// exit closed mode
// it it's from somebody else, ignore it
// if we've been in closed mode past `timeout` time
// exit closed mode
// otherwise exit closed mode
// otherwise ignore it




#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SerialFlash.h>

#define SERVER_ADDRESS 1

// Define pins
#define RFM95_CS 5
#define RFM95_INT 2
#define LED 13

#define Serial SerialUSB

// Define frequency
#define RF95_FREQ 915.0

const int FlashChipSelect = 4; // digital pin for flash chip CS pin

uint32_t testsize = 512; // test file size
int busyWithClient = -1; // flag -1 if not currently engaged with a client, otherwise will be client ID when engaged

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);


/****** setup ******/
void setup()
{
  // Wait for serial port to be available
  while (!Serial);

  // start serial comms with the computer
  Serial.begin(9600);

  // while the radio manager hasn't inited
  while (!manager.init()) {
  }

  // print surccess
  Serial.println("Server init success");

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);

  // Start SerialFlash
  if (!SerialFlash.begin(FlashChipSelect)) {
    while (1) {
      Serial.println("Unable to access SPI Flash chip");
      delay(1000);
    }
  }
  Serial.println("Able to access SPI flash chip");
  SerialFlash.wakeup();

  //Erase everything
  Serial.println("Erasing everything...");
  SerialFlash.eraseAll();
  while (SerialFlash.ready() == false) {
    // wait, 30 seconds to 2 minutes for most chips
  }
  Serial.println("Done erasing everything");
  Serial.println();
}


void loop() {

  // Serial.println("Waiting for Godot...");
  // Dont put this on the stack:
  uint8_t buf[30];

  // get a buffer to hold a filename
  char filename[sizeof(buf)];

  // if the manager isn't busy right now
  if (manager.available()) {

    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAck(buf, &len, &from)) {

      // if we're not already busy with a client
      if (busyWithClient == -1) {

        // save the client id
        busyWithClient = from;

        // and a response buffer
        uint8_t* handshake;

        // print the handshake message (filename)
        Serial.print("got request from: ");
        Serial.print(from, DEC);
        Serial.print(": ");
        // Serial.println((char*) buf);
        // Serial.println("made it here2: ");
        strcpy(filename, (char*)buf);
        // strcat(filename, " ");
        Serial.println(filename);
        // Serial.println("made it here3: ");

        // if we've got the file here on the server
        if (SerialFlash.exists(filename)) {
          // open it
          SerialFlashFile file;
          file = SerialFlash.open(filename);

          // convert number of bytes we've received to char and store in buffer
          int numBytesReceived = file.position();
          itoa(numBytesReceived,(char*)handshake,10);

          Serial.print("bytes received: ");
          Serial.println(numBytesReceived,DEC);

        }
        else { // otherwise

          // create the file here
          if(SerialFlash.create(filename, 512)) {
            Serial.print("Created: ");
            Serial.println(filename);

            // open it
            SerialFlashFile file;
            file = SerialFlash.open(filename);

            // convert number of bytes we've received to char and store in buffer (should be 0)
            int numBytesReceived = file.position();
            itoa(numBytesReceived,(char*)handshake,10);

            Serial.print("bytes received: ");
            Serial.println(numBytesReceived,DEC);

          }
          else{
            Serial.println("failed to create file");
          }
        }

        // send the handshake message back to the client
        Serial.print("Sending handshake back to client: ");
        Serial.println(*handshake);


        // Send a reply back to the originator client
        if (!manager.sendtoWait(handshake, sizeof(handshake), from)) {
          Serial.println("Client failed to acknowledge reply");
        }
      }
      else if(from == busyWithClient) { // if the message is from the client we're busy with

      // get a buffer to hold the data
      char dataToWrite[sizeof(buf)];

      // copy the received data into a write buffer
      Serial.print("Writing to file: ");
      strcpy(dataToWrite, (char*)buf);
      strcat(dataToWrite, " ");
      Serial.println(dataToWrite);
      Serial.println();

      // open the file
      SerialFlashFile file;
      file = SerialFlash.open(filename);

      // write the data to the file
      const uint32_t wrlen = sizeof(dataToWrite); // This must be const
      file.write(dataToWrite, wrlen);
      file.close();

      // return to wait mode
      busyWithClient = -1;
    }
    // in all other cases, ignore sender
  }
}
}

void spaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(' ');
  }
}
