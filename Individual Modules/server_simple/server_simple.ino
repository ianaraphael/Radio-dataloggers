// rf95_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple, addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_client
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W


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

#define SERVER_ADDRESS 1

// Define pins
#define RFM95_CS 5
#define RFM95_INT 2
#define LED 13

#define Serial SerialUSB

// Define frequency
#define RF95_FREQ 915.0

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

  // while the radio manager hasn't init'ed
  while (!manager.init()) {
  }

  // print surccess
  Serial.println("Server init success");

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
}


void loop() {
  // uint8_t data[] = "Hey there! Good to hear from you.";
  uint8_t data[] = "0";

  // Dont put this on the stack:
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char msg[sizeof(buf)];
  buf[0] = '\0';

  // if the manager isn't busy right now
  if (manager.available()) {

    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (manager.recvfromAck(buf, &len, &from)) {
      Serial.print("got request from: ");
      Serial.print(from, DEC);
      Serial.print(": ");
      strcpy(msg, (char*)buf);
      strcat(msg, " ");
      Serial.println(msg);

      Serial.println("Sending reply back to client");

      // Send a reply back to the originator client
      if (!manager.sendtoWait(data, sizeof(data), from)) {
        Serial.println("Client failed to acknowledge reply");
      }
    }
  }
}
