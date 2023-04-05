/*
  Script for testing interrupt for a contoller/responder (SIMB/sensor controller model)
  This is the controller, just sends a pin high to interrupt the SC
*/
#define CS 5

void setup() {
  pinMode(CS,OUTPUT);
  digitalWrite(CS,HIGH);
}

void loop() {

  // wait five seconds
  delay(5000);

  // send pin high
  digitalWrite(CS,LOW);

  // wait five seconds
  delay(5000);

  // send pin low
  digitalWrite(CS,HIGH);
}
