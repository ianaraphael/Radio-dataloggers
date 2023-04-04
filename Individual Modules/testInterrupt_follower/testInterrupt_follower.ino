/*
  Script for testing interrupt for a contoller/responder (SIMB/sensor controller model)
  This is the responder, waits for a pin to go change then does something
*/
volatile bool INTERRUPTED = false;
volatile int currState = LOW;

void setup() {

  SerialUSB.begin(9600);

  delay(5000);

  // pullup pin 2
  pinMode(2,INPUT_PULLUP);

  // attach an interrupt to trigger on a low pin
  attachInterrupt(digitalPinToInterrupt(2), simbInterruptHandler, LOW);
}

void loop() {
  delay(5000);

  // attach the interrupt
  attachInterrupt(digitalPinToInterrupt(2), simbInterruptHandler, LOW);
}

void simbInterruptHandler(void) {

  // do something
  SerialUSB.println("Interrupt triggered");

  // detach the interrupt so we don't get stuck in a loop
  detachInterrupt(digitalPinToInterrupt(2));
}

// void callOnInterrupt(){
//   SerialUSB.println("And I can call a second fx");
//
//   INTERRUPTED = true;
// }
