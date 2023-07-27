/*

client_ArcWatch_v1

bytestream based client for ArcWatch deployment. Eliminates local files, human
readable text

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.06.16

*/

#include "SnowTATOS.h"

#define Serial SerialUSB // comment if not using rocketscream boards

void setup() {

  boardSetup();

  // Begin serial comms
  Serial.begin(9600);
  delay(5000);

  // init the temperature sensors
  initTemps();

  // init the radio
  init_Radio();

  // init the realtime clock
  init_RTC();

  Serial.print("Station #");
  Serial.print(STATION_ID,DEC);
  Serial.println(" init'd");
}



void loop() {

  // read the data
  uint8_t pingerData = readPinger();
  static float tempData[NUM_TEMP_SENSORS];
  readTemps(tempData);

  // print out the temp data
  Serial.println("temp data: ");
  for (int i=0;i<NUM_TEMP_SENSORS;i++) {
    Serial.println(tempData[i],3);
  }
  Serial.println("pinger data: ");
  Serial.println(pingerData);

  // allocate a buffer
  uint8_t dataBuffer[CLIENT_DATA_SIZE];

  // and pack it
  packClientData(tempData, pingerData, dataBuffer);

  Serial.println("Here's the data we're sending: ");
  for (int i=0;i<CLIENT_DATA_SIZE;i++){
    Serial.print(" 0x");
    Serial.print(dataBuffer[i],HEX);
  }
  Serial.println("");

  Serial.println("Attempting radio transmit");

  // send the data
  if(sendData_fromClient(dataBuffer)){
    Serial.println("Successful transmit");
  } else {
    Serial.println("Failed transmit");
  }

  // set the alarm
  setAlarm_client();
  sc_RTC.standbyMode();
}
