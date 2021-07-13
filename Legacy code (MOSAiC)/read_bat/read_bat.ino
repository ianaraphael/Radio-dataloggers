/* read_bat
 *  Basic sketch to demonstrate reading the battery voltage, note this is same 
 *  procedure as reading analog data
 */

// Variables
//int adcReading;
unsigned short adcReading;
unsigned char high;
unsigned char low;
unsigned char counter;
float batteryVoltage;

void setup() {
  // In this case we'll just be writing the battery voltage out to the USB
  // so we need a serial connection
  SerialUSB.begin(9600);

  // Set the analog resolution to use the full 12 bits
  analogReadResolution(12);
}

void loop() {
  //1st reading is inaccurate for some reason?
  adcReading = analogRead(A5); //bat monitor is on pin A5
  // discard first reading
  adcReading = 0;
  adcReading = analogRead(A5);
  // perform averaging
//  for (counter=0; counter < 10; counter++)
//  {
//    adcReading += analogRead(A5);
//  }
//  adcReading = adcReading / 10;
  // Convert to volts
  batteryVoltage = adcReading * (4.3 / 1023.0);

  // Write to serial
  SerialUSB.print(adcReading, BIN);
  SerialUSB.print(" ");
  SerialUSB.print(highByte(adcReading), BIN);
  SerialUSB.print(" ");
  SerialUSB.print(lowByte(adcReading), BIN);
  //Serial.print("Hi");
  SerialUSB.println();

  // Wait 1 second before repeating
  delay(1000);
}
