/*

readWrite.ino

read and write data line by line from/to the flash chip

Ian Raphael
ian.th@dartmouth.edu
2021.07.19

*/

#include <SerialFlash.h>
#include <SPI.h>

#define Serial SerialUSB

const int FlashChipSelect = 4; // digital pin for flash chip CS pin

void setup() {

  // Set input_pullup to disable SPI comms to radio
  pinMode(5, INPUT_PULLUP);

  // Start serial and wait for serial monitor
  Serial.begin(9600);
  while (!Serial);
  delay(100);
  Serial.println("Serial comms running");

  // Start communicating with the flash chip
  while (!SerialFlash.begin(FlashChipSelect)) {
    Serial.println("Unable to access SPI Flash chip");
    delay(1000);
  }
  Serial.println("Able to access SPI flash chip");

  // Display ID and capacity
  unsigned char flashID[5];
  SerialFlash.readID(flashID);
  Serial.print("Chip ID: ");
  for (int i=0; i<5; i++) {
    Serial.print(flashID[i]);
    Serial.print(" ");
  }
  Serial.println(" ");
  unsigned long size = SerialFlash.capacity(flashID);
  Serial.print("Flash memory capacity (bytes): ");
  Serial.println(size);

  //Erase everything
  Serial.println("Erasing everything:");
  SerialFlash.eraseAll();
  Serial.println("Done erasing everything");

  // Create a file
  const char *filename = "test.bin";
  uint32_t testsize = 256;
  SerialFlash.create(filename, testsize);
  Serial.print("Created: ");
  Serial.println(filename);

  // Check if the file we just created exists
  Serial.print("Does the file exist? ");
  Serial.println(SerialFlash.exists(filename));

  // Open the file and check how much space is available
  SerialFlashFile file;
  file = SerialFlash.open(filename);
  if (file) {
    Serial.print("File size (bytes): ");
    Serial.println(file.size());
    Serial.print("File position: ");
    Serial.println(file.position());
    Serial.print("Available space (bytes): ");
    Serial.println(file.available());
  } else {
    Serial.print("Failed to open ");
    Serial.println(filename);
  }

  // Write something to the file
  const uint32_t wrlen = 256; // This must be const
  char buf[wrlen];
  for (int i=0; i<wrlen; i++) {
    buf[i] = i;
  }
  Serial.print("Buffer to write to file: ");
  for (int i=0; i<wrlen; i++) {
    Serial.print((int)buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
  file.write(buf, wrlen);
  Serial.println("Wrote buffer to file");
  Serial.print("Now available space is: ");
  Serial.println(file.available());
  file.close();

  // Read back from file
  char buf2[testsize];
  SerialFlashFile file2;
  file2 = SerialFlash.open(filename);
  Serial.println("Reading from file:");
  file2.read(buf2, testsize);
  for (int i=0; i<testsize; i++) {
    Serial.print((int)buf2[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
}

void loop() {


}
