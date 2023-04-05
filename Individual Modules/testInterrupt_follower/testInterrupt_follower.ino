/*
  Script for testing interrupt for a contoller/responder (SIMB/sensor controller model)
  This is the responder, waits for a pin to go change then does something
*/
volatile bool INTERRUPTED = false;
volatile int currState = LOW;

#define CS 5

void setup() {

  SerialUSB.begin(9600);

  delay(5000);

  // pullup pin 2
  pinMode(CS,INPUT_PULLUP);

  // attach an interrupt to trigger on a low pin
  attachInterrupt(digitalPinToInterrupt(CS), simbInterruptHandler, LOW);

  SerialUSB.println("I'm here");
}

void loop() {
  delay(5000);

  // attach the interrupt
  attachInterrupt(digitalPinToInterrupt(CS), simbInterruptHandler, LOW);

  if (INTERRUPTED) {
    // do something
    SerialUSB.println("Interrupt triggered");

    // reset flag
    INTERRUPTED = false;
  }
}

void simbInterruptHandler(void) {

  INTERRUPTED = true;

  // detach the interrupt so we don't get stuck in a loop
  detachInterrupt(digitalPinToInterrupt(CS));
}

// void callOnInterrupt(){
//   SerialUSB.println("And I can call a second fx");
//
//   INTERRUPTED = true;
// }
