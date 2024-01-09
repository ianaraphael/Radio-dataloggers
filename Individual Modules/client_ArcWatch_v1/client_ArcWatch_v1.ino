/*

client_ArcWatch_v2

bytestream based client for ArcWatch deployment. Eliminates local files, human
readable text.

Updated 2023.11.28 for Rocket scream mini ultra boards

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.11.28

*/

// **************************** defines, macros **************************** //
#include "SnowTATOS.h"

#define Serial Serial2

#if (STATION_ID == 0)
  #error Client STATION_ID must not be 0!
#endif

// ********************************* setup ********************************* //
void setup() {

  boardSetup();

  // Begin serial comms
  Serial.swap(1);
  Serial.begin(9600);
  delay(5000);

  // init the temperature sensors
  initTemps();

  // init the realtime clock
  init_RTC();

  // init the radio
  init_Radio();

  Serial.print("Station #");
  Serial.print(SIMB_ID,DEC);
  Serial.print("_");
  Serial.print(STATION_ID,DEC);
  Serial.println(" initialized succesfully.");

  // set this false on startup so that any time we black out we reattempt sync
  syncedWithServer = false;

  // start listening for radio traffic
  radio.startReceive();
}


// ******************************* main loop ******************************* //
void loop() {

  Serial.println("Just woke up! Attempting to sync with server...");

  bool explicitSync;
  // while we're not synced up with the server
  while (!syncedWithServer) {
    // attempt to sync up with the server
    explicitSync = attemptSyncWithServer();
  }

  // set the next sampling alarm
  setSleepAlarm(SAMPLING_INTERVAL_MIN);

  // print hard/soft sync
  if (explicitSync) {
    Serial.println("Explicitly synced with server");
  } else {
    Serial.println("Implicitly synced with server");
  }

  // get the battery voltage
  float voltage = readBatteryVoltage();

  // read the pinger data
  uint16_t pingerData = readPinger();

  static float tempData[NUM_TEMP_SENSORS];
  // if there are any temp sensors
  if (NUM_TEMP_SENSORS > 0){
    // read them
    readTemps(tempData);
    // print out the temp data
    Serial.println("temp data: ");
    for (int i=0;i<NUM_TEMP_SENSORS;i++) {
      Serial.println(tempData[i],3);
    }
  }

  // print off
  Serial.print("pinger data: ");
  Serial.println(pingerData);

  Serial.print("voltage: ");
  Serial.println(voltage,3);

  // allocate a buffer
  uint8_t dataBuffer[CLIENT_DATA_SIZE];

  // and pack it
  packClientData(tempData, pingerData, voltage, dataBuffer);

  Serial.println("Here's the data we're sending: ");
  for (int i=0;i<CLIENT_DATA_SIZE;i++){
    Serial.print(" 0x");
    Serial.print(dataBuffer[i],HEX);
  }
  Serial.println("");

  // send the data
  Serial.println("Attempting radio transmit");
  int transmitErrorState = sendData_fromClient(dataBuffer,CLIENT_DATA_SIZE);

  if(transmitErrorState == 0){
    Serial.println("Successful transmit");
  } else {
    Serial.print("Failed transmit, error ");
    Serial.println(transmitErrorState);
  }

  // if we've exceeded the number of allowable failed transmits
  if (nFailedTransmits >= MAX_FAILED_TRANSMITS_TO_SYNC) {
    // reset our sync flag
    syncedWithServer = false;
  }

  // if we're synced up with the server
  if (syncedWithServer) {

    Serial.print("Going to sleep for ");
    Serial.print(SAMPLING_INTERVAL_MIN,DEC);
    Serial.println(" minutes");

    // put the radio to sleep
    radio.sleep();

    delay(1000);

    // go to sleep
    LowPower.standby();
  }
}
