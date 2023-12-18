/*

client_ArcWatch_v2

bytestream based client for ArcWatch deployment. Eliminates local files, human
readable text.

Updated 2023.11.28 for Rocket scream mini ultra boards

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.11.28

*/

#include "SnowTATOS.h"

#define Serial SerialUSB // comment if not using rocketscream boards

#define CLIENT_TRANSMIT_DELAY_SECS 2 // wait a few seconds after waking before trying to transmit to server

void setup() {

  boardSetup();

  // Begin serial comms
  Serial.begin(9600);
  delay(5000);

  // init the temperature sensors
  initTemps();

  // init the realtime clock
  init_RTC();

  // TODO: set the radio pin
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  // init the radio
  init_Radio();

  Serial.print("Station #");
  Serial.print(STATION_ID,DEC);
  Serial.println(" init'd");

  // set this false on startup so that any time we black out we reattempt sync
  syncedWithServer = false;
}



void loop() {

  // TODO: set the radio pin
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  // init the radio
  init_Radio();

  // while we're not synced up with the server
  while (!syncedWithServer) {

    // attempt to sync up with the server
    syncedWithServer = attemptSyncWithServer();

    // if successful
    if (syncedWithServer){
      // break out
      break;
    }

    // otherwise go to sleep for an interval half as long as the server wake duration
    if (SERVER_WAKE_DURATION > 1) {
      setSleepAlarm(SERVER_WAKE_DURATION/2);
      LowPower.standby();
    }
  }

  // set the next sampling alarm
  setSleepAlarm(SAMPLING_INTERVAL_MIN);

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

  // delay for a couple of second
  delay(CLIENT_TRANSMIT_DELAY_SECS*1000);

  // send the data
  Serial.println("Attempting radio transmit");

  if(manager.sendtoWait(dataBuffer,CLIENT_DATA_SIZE, SERVER_ADDRESS)){
    Serial.println("Successful transmit");
  } else {
    Serial.println("Failed transmit");
  }

  // go to sleep
  LowPower.standby();
}
