/*

readPinger.ino

Read and print snow pinger digital output with output error checking and non-return
protection

Ian Raphael
ian.th@dartmouth.edu
2021.07.19

*/

#define Serial SerialUSB
#define PINGER_BUS Serial1
#define PINGER_TIMEOUT 100

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("USB Comms online");

  PINGER_BUS.begin(9600);
}

void loop() {
  // If we're connected to the PINGER_BUS get a reading. Since the pinger is constantly
  // spewing data when powered we only need the RX pin (we never TX to pinger)
  if (PINGER_BUS.available()) {

    String pingerReadout; // a string to hold what's read off the pinger
    String pingerReturn; // a string to return the cleaned pinger info

    // TODO: Include a timer here to protect against errors. We don't want to get into
    // a situation where the pinger is just spitting back garbage (so Serial1.available() still
    // returns true) but this just hangs the board since it never spits out '\r'
    // and possibly runs us out of memory.
    // set a timeout for reading the serial line
    Serial.setTimeout(PINGER_TIMEOUT);

    // The format of the pingers output is ascii: Rxxxx\r where x is a digit
    // Thus we can read the streaming input until we catch '\r'
    // get the pinger readout
    pingerReadout = PINGER_BUS.readStringUntil('\r');

    // copy the pinger readout into the return string starting after the first char
    pingerReturn = pingerReadout.substring(1);

    // if the readout doesn't conform to the R + 4 digits format
    if ((pingerReadout[0] != 'R') || (pingerReadout.length()!= 5)) { // first check if the first char is caps R

      // set the output to string "NaN"
      pingerReturn = "NaN";

    } else {
      // then for the following four chars
      for (int i=1; i<5; i++) {

        // if any of them aren't digits
        if (!isdigit(pingerReadout[i])) {

          // set the output to string "NaN"
          pingerReturn = "NaN";

          // and escape
          break;
        }
      }
    }

    Serial.print("Digital read (string, mm): ");
    Serial.println(pingerReturn);

    Serial.print("Digital read (int, mm): ");
    unsigned long pingerReturn_long_mm = pingerReturn.toInt();
    Serial.println(pingerReturn_long_mm,DEC);

    // convert the pinger reading to cm
    float pingerReturn_float_cm = (float)pingerReturn_long_mm/10;
    Serial.print("Digital read (float, cm): ");
    Serial.println(pingerReturn_float_cm,DEC);

    int pingerReturn_int_cm = (int) round(pingerReturn_float_cm);
    Serial.print("Digital read (int, cm): ");
    Serial.println(pingerReturn_int_cm,DEC);
  }

  // Serial.print(" ");
  // The brown wire outputs the distance in analog. Using the arduino's native
  // 10 bit adc resolution the distance is approximately the analog value * 5
  // Serial.println(analogRead(A1)*5);
  delay(500);

}
