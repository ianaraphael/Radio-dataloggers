/*

  A library for handling file functions in temperature datalogger
  deployments. creates, reads, writes generic datafiles given a filename and
  header information. based on SD library

  Ian Raphael
  2023.03.27
  ian.a.raphael.th@dartmouth.edu
  
*/

#ifndef dataFile_h
#define dataFile_h

#include "Arduino.h"
#include "SD.h"


// create a new file if it doesn't exist
void newFile(String filename, String* headerInformation, int numHeaderLines) {

  // if the file doesn't already exist
  if (!SD.exists(filename)) {

    Serial.print("Creating datafile: ");
    Serial.println(filename);

    File dataFile = SD.open(filename, FILE_WRITE);

    // write the header information
    // if the file is available
    if (dataFile) {

      // print the header information to the file
      for (int i = 0; i < numHeaderLines; i++) {

        dataFile.println(headerInformation[i]);

        // also print to serial to confirm
        Serial.println(headerInformation[i]);
      }

      // close the file
      dataFile.close();
    }
  }
}

/************ writeToFile ************/
/*
function for writing generic data to generic file
*/
void writeToFile(String filename, String dataString) {

  // open the file for writing
  File dataFile = SD.open(filename, FILE_WRITE);

  // if the file is available
  if (dataFile) {

    // write the data
    dataFile.println(dataString);

    // close the file
    dataFile.close();

    // flash LED to indicate successful data write
    for (int i = 0; i < 5; i++) {
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
    }
  }
}

/************ init_SD ************/
void init_SD() {

  delay(100);
  // set SS pins high for turning off radio
  pinMode(RADIO_CS, OUTPUT);
  delay(500);
  digitalWrite(RADIO_CS, HIGH);
  delay(500);
  pinMode(SD_CS, OUTPUT);
  delay(1000);
  digitalWrite(SD_CS, LOW);
  delay(2000);
  SD.begin(SD_CS);
  delay(2000);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
}

#endif
