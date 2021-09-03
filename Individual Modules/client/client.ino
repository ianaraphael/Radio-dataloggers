// client.ino

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SerialFlash.h>

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

const int FlashChipSelect = 4; // digital pin for flash chip CS pin

const char *filename = "test.txt"; // filename
int numBytesTransmitted = 0; // number of bytes already transmitted
int numBytesRecorded = 0; // number of bytes recorded
uint32_t testsize = 256; // test file size
int lineSize = 2; // the size of a line of data. rn it's just a character per line, plus newline

/****** setup ******/
void setup()
{

  // Wait for serial port to be available
  while (!Serial);

  // start serial comms with the computer
  Serial.begin(9600);

  /**** Radio ****/
  // while the radio manager hasn't inited
  while (!manager.init()) {
  }

  // print surccess
  Serial.println("Client radio init success");

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);


  /**** flash chip ****/

  // Start SerialFlash
  if (!SerialFlash.begin(FlashChipSelect)) {
    while (1) {
      Serial.println("Unable to access SPI Flash chip");
      delay(1000);
    }
  }
  Serial.println("Able to access SPI flash chip");

  //Erase everything
  Serial.println("Erasing everything:");
  SerialFlash.eraseAll();
  Serial.println("Done erasing everything");

  // Create a file
  SerialFlash.create(filename, testsize);
  Serial.print("Created: ");
  Serial.println(filename);

  // open it
  SerialFlashFile file;
  file = SerialFlash.open(filename);

  // write something to it
  char buf[] = "a";
  const uint32_t wrlen = sizeof(buf); // This must be const
  numBytesRecorded += wrlen; // update the number of bytes recorded
  Serial.print("Write to file: ");
  Serial.println(buf);
  file.write(buf, wrlen);
  file.close();
}

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

/****** main ******/
void loop() {

  // open the file
  SerialFlashFile file2;
  file2 = SerialFlash.open(filename);

  // get something to write
  char writeThis[] = "a";
  const uint32_t wrlen = sizeof(writeThis); // This must be const
  Serial.print("Write to file: ");
  Serial.println(writeThis);
  file2.write(writeThis, wrlen);
  file2.close();
  // increment numBytesRecorded
  numBytesRecorded += wrlen;

  // we're going to send the filename that we want to transmit as the handshake
  uint8_t* handshake = (uint8_t*) filename;

  // allocate a buffer to hold received uint data and one for char data
  uint8_t receiveBuf[RH_RF95_MAX_MESSAGE_LEN];
  char msg[sizeof(receiveBuf)];

  Serial.println("Attempting to handshake with the server");

  // send the initialize handshake message to the server
  if (manager.sendtoWait(handshake, sizeof(handshake), SERVER_ADDRESS)) {

    // Now wait for the reply from the server
    uint8_t len = sizeof(receiveBuf);
    uint8_t fromID;
    // if we receive a message
    if (manager.recvfromAckTimeout(receiveBuf, &len, TIMEOUT, &fromID)) {

      // open the file
      file2 = SerialFlash.open(filename);

      // the handshake message should be the number of lines the server has already received
      strcpy(msg, (char*)receiveBuf); // convert received buffer to a string TODO: can we go directly to int?
      int numBytesReceived = (String((char*)receiveBuf)).toInt(); // then to an int

      Serial.print("Server has already received this many bytes: ");
      Serial.println(numBytesReceived);

      // seek to the next line after it
      // (sent/received: 0, attempt send line 0)
      // (sent/recieved: 1, attempt send line 1)
      // (because it's 0 indexed)
      file2.seek(numBytesReceived);

      // get a buffer
      char sendBuf[(numBytesRecorded - numBytesReceived)];

      // read the untransmitted data into the buffer
      Serial.println("Reading from file:");
      file2.read(sendBuf, sizeof(sendBuf));

      // send that to the server
      manager.sendtoWait((uint8_t*)sendBuf, sizeof(sendBuf), SERVER_ADDRESS);

    }
    else {
      Serial.println("Server's busy, going about my business");
    }
  }
  else {
    Serial.println("Server failed to acknowledge handshake");
  }
  delay(1000);
}
