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
#include <SD.h>

#define SERVER_ADDRESS 1

// Define pins
#define RADIO_CS 5
#define RADIO_INT 2
#define LED 13
#define SD_CS 10 // SD card chip select
#define SD_CD NAN // SD card chip DETECT. Indicates presence/absence of SD card. High when card is inserted.

#define Serial SerialUSB

// Define frequency
#define RADIO_FREQ 915.0

// indicates whether we're with a client and which client it is. default to server_address when on standby.
int volatile withClient = SERVER_ADDRESS;

// declare a global file object to write stuff onto
File dataFile;
// declare a global filename
String filename;

// Singleton instance of the radio driver
RH_RF95 driver(RADIO_CS, RADIO_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);


/****** setup ******/
void setup()
{
  // Wait for serial port to be available
  while (!Serial);

  // start serial comms with the computer
  Serial.begin(9600);

  // while the radio manager hasn't init'ed
  while (!manager.init()) {
  }

  // print surccess
  Serial.println("Server init success");

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RADIO_FREQ);
}


void loop() {
  // // uint8_t data[] = "Hey there! Good to hear from you.";
  // uint8_t numBytesServer[] = "0";

  // Dont put this on the stack:
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char msg[sizeof(buf)];
  buf[0] = '\0';

  // if the manager isn't busy right now
  if (manager.available()) {

    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;

    // if we've gotten a message
    if (manager.recvfromAck(buf, &len, &from)) {

      // if the message is from a client we're already dealing with or if we're in standby mode
      if (from == withClient || withClient == SERVER_ADDRESS) {

        // get the first char
        uint8_t messageType = buf[0];

        // switch on the message contents
        switch (messageType) {

          // if it's a transaction begin/end message
          case 0:

          // if it's from a client we're already dealing with
          if (withClient == from) {

            // it's a transaction termination message; switch withClient back to server address to indicate "standby" mode
            withClient = SERVER_ADDRESS;

            Serial.print("transaction with: ");
            Serial.print(from, DEC);
            Serial.println(" terminated.");

            // close the file
            dataFile.close();

          } else if (withClient == SERVER_ADDRESS) { // if we're in standby mode

          // it's a transaction initiation message; change withClient to remember which client we're talking to
          withClient = from;

          // initialize the SD card
          init_SD();

          // get the filename
          filename = "";
          filename = String((char*)&buf[1]);

          // open the appropriate file
          dataFile = SD.open(filename, FILE_WRITE);

          // figure out how many bytes we have
          unsigned long numBytesServer = dataFile.size();

          // and send a handshake back to the client
          Serial.println("Sending handshake back to client: ");
          Serial.println(from, DEC);

          // Send a reply back to the originator client
          if (!manager.sendtoWait((uint8_t *) &numBytesServer, sizeof(&numBytesServer), from)) {
            Serial.println("Client failed to acknowledge reply");
          }
        }
        break;

        // /******************** add case to deal with getting the filename and n bytes from client ********************/
        // // something like: first two bytes are case code, then numbytes, then filename
        // case 1:



        // // by default
        // default:

        case 1:

        // print the data out
        Serial.print((char*) &buf[1]);

        // if the file is available
        if (dataFile) {

          // write the data to the file
          dataFile.print(String((char*) &buf[1]));
            // flash LED to indicate successful data write
            // for (int i=0;i<5;i++){
            //   digitalWrite(13,HIGH);
            //   delay(500);
            //   digitalWrite(13,LOW);
            //   delay(500);
            // }
        }
        break;

        // default:
        // ...do something...
      }
    }
  }
}
}

/************ init_SD ************/
void init_SD(){

  delay(100);
  // set SS pins high for turning off radio
  pinMode(RADIO_CS, OUTPUT);
  delay(500);
  digitalWrite(RADIO_CS, HIGH);
  delay(500);
  pinMode(SD_CS, OUTPUT);
  delay(500);
  digitalWrite(SD_CS, LOW);
  delay(1000);
  SD.begin(SD_CS);
  delay(1000);
}
