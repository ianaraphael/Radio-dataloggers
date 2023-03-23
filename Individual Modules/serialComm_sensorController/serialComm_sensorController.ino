/*

serialComm_sensorController.ino

Sensor controller side code for serial communication with SIMB3

Ian Raphael
ian.th@dartmouth.edu
2023.03.13
*/

#include <SoftwareSerial.h>

const byte rxPin = 3;
const byte txPin = 4;

#define SIMBINTERRUPT 2

void setup() {

  // Begin serial monitor comms
  SerialUSB.begin(9600);

  pinMode(2,INPUT_PULLDOWN);

  // set the interrupt
  attachInterrupt(digitalPinToInterrupt(2), simbInterruptHandler, FALLING);

  // Set up a new SoftwareSerial object
  SoftwareSerial mySerial (rxPin, txPin);
}

void loop() {

}

void simbInterruptHandler(void) {
  SerialUSB.println("Interrupt triggered");
  callOnInterrupt();
}

void callOnInterrupt(){

    // Begin serial comms with SIMB
    mySerial.begin(9600);

    SerialUSB.println("Established connection");
    SerialUSB.println("Sending data to SIMB");

    // make up some data
    char returnData[] = "Hello SIMB";

    // send the data length over to the SIMB
    mySerial.print(sizeof(returnData));

    // then send the message
    mySerial.print(returnData);
}
