/*
 * provides a brief example of reading the serial output and analog output of a maxbotix pinger
 * !!!!!!may only work with mini ultra pro board
 *
 * Wires coming out of pinger:
 * Red: 3V3 (Vin)
 * Black: GND
 * Brown: A1 outputs distance as scaled Analog voltage between GND and Vin (Vin corresponds to 4999 mm)
 * Blue: 0 (PINGER_BUS's RX)
 * White: Not used, probably pin 1 temperature sensor input
 * Orange: not used, PWM output
 * Green: not used, pull low to turn off, pull high to turn on. Internally pulled high.
 */

#define Serial SerialUSB
#define PINGER_BUS Serial


String pingerOutput;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("USB Comms onine");

  PINGER_BUS.begin(9600);
}

void loop() {
  // If we're connected to the PINGER_BUS get a reading. Since the pinger is constantly
  // spewing data when powered we only need the RX pin (we never TX to pinger)
  if (PINGER_BUS.available()) {
    // The format of the pingers output is ascii: Rxxxx\r where x is a digit
    // Thus we can read the streaming input until we catch '\r'
    // TODO: check that string matches 'R'[0-9][0-9][0-9][0-9]'\r' and if not discard it
    // TODO: Include a timer here to protect against errors. We don't want to get into
    // a situation where the pinger is just spitting back garbage (so Seral1.available() still
    // returns true) but this just hangs the microcontroller since it never spits out '\r'
    // and possibly runs us out of memory.
    pingerOutput = PINGER_BUS.readStringUntil('\r');
    Serial.print(pingerOutput);
  }

  Serial.print(" ");
  // The brown wire outputs the distance in analog. Using the arduino's native
  // 10 bit adc resolution the distance is approximately the analog value * 5
  Serial.println(analogRead(A1)*5);
  delay(500);

}
