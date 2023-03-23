/*

serialComm_simb.ino

SIMB3 side code for software serial communication with Ian's sensor controller

Ian Raphael
ian.th@dartmouth.edu
2023.03.13
*/

#include <SoftwareSerial.h>

const byte rxPin = 3;
const byte txPin = 4;

#define sensorController_CS 2 // use pin 2 as the sensor controller chip select

void setup() {

  // Begin serial monitor comms
  SerialUSB.begin(9600);

  // set SC CS pin low
  pinMode(sensorController_CS,OUTPUT);
  digitalWrite(sensorController_CS,HIGH);

  // Set up a new SoftwareSerial object
  SoftwareSerial mySerial (rxPin, txPin);

}


void loop() {

  // delay for 5 seconds
  delay(5000);

  // set sensor controller chip select pin high
  digitalWrite(sensorController_CS,LOW);

  // connect to SC
  mySerial.begin(9600);

  SerialUSB.println("Requesting data from sensor controller");

  // wait until the SC has responded
  // TODO: this is not safe. add a timeout
  while(!mySerial.available()) {
  }

  // then ask how many bytes the SC has for us
  int size_dataToReceive = mySerial.parseInt();

  SerialUSB.print("SC has ");
  SerialUSB.print(size_dataToReceive,DEC);
  SerialUSB.println(" bytes of data to send.");

  // Allocate a buffer to hold the data
  char dataReceived[size_dataToReceive+1];

  // now read until there's no data left
  int i = 0;
  while(mySerial.available()){
    dataReceived[i] = (mySerial.read());
    i++;
  }

  // now conclude transmission with SC
  digitalWrite(sensorController_CS,HIGH);

  // Print the data to serial monitor
  SerialUSB.print("Received data from SC: ");
  SerialUSB.println(dataReceived);
}
