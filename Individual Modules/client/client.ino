// rf95_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_server
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W




//**** client pseudocode ****//

// say hi to the server. attempt n times
  // message with:
  // "hi, my ID is: X
  // I'd like to begin transmitting these files: "
  // filename 1:
  // filename 2:
// if no response after n attempts, go back to sleep
// elseif we get an affirmative response, the server is locked in for us
  // server will tell us the number of lines of each file that it has previously received/recorded
  // send the new lines (attempt n times) starting just after that last line of file that's already been received, until done
// go back to sleep



#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define SERVER_ADDRESS 1
#define CLIENT_ADDRESS 2
#define TIMEOUT 2000


// Define pins
#define RFM95_CS 5
#define RFM95_INT 2
#define LED 13

#define Serial SerialUSB

// Define frequency
#define RF95_FREQ 915.0


// ***** globals ***** //
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);


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
  Serial.println("Client init success");

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
}


/****** main ******/
void loop() {

  // make up some data
  uint8_t data[] = "Hello World!";

  // Dont put this on the stack:
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char msg[sizeof(buf)];

  Serial.println("Attempting to send a message to the server");

  // send the initialize handshake message to the server
  if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS)) {

    // Now wait for the reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAckTimeout(buf, &len, TIMEOUT, &from)) {

      // print whatever the server sent back
      strcpy(msg, (char*)buf);
      strcat(msg, " ");
      Serial.println(msg);

    }
    else
    {
      Serial.println("Server's busy, going about my business");
    }
  }
  else {
    Serial.println("Server failed to acknowledge receipt");
  }
  delay(1000);
}
