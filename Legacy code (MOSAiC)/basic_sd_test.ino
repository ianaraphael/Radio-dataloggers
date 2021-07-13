// test logging script

#include <SPI.h>
#include <SD.h>

// Set the pin used for selecting the sd card serial device
#define cardSelect 4

// Create a logfile to write to
File logfile;

// blink out an error code
void error(uint8_t errno) {
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

void setup() {
  // connect to computer serial just to see what's happening
  while (!Serial) ; // wait for serial to start
  Serial.begin(9600);
  Serial.println("Starting");
  pinMode(13, OUTPUT);


  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "/ANALOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[7] = '0' + i/10;
    filename[8] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to "); 
  Serial.println(filename);

  pinMode(13, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.println("Ready!");
}

uint16_t counter = 0;

void loop() {
  // Let's just have it write a loop for now
  logfile.print(millis());
  logfile.print(", ");
  logfile.println(counter);
  logfile.flush();
  Serial.print(millis());
  Serial.print(", ");
  Serial.println(counter);
  delay(1);
  counter++;

}
