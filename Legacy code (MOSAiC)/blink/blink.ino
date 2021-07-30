/* Blink
 *  Simple program to check functionality of rocketscream mini ultra pro
 */

void setup() {
  // initialize digital pin 13 (orange LED) as an output
  pinMode(13, OUTPUT);
}

void loop() {
  // Turn LED on and off every second
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
}
