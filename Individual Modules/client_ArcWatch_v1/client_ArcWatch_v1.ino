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

  boardSetup(unusedPins_client);

  if (PRINT_SERIAL) {
    // Begin serial comms
    Serial.swap(1);
    Serial.begin(9600);
    delay(5000);
  }

  // init the temperature sensors
  initTemps();

  // init the realtime clock
  init_RTC();

  // init the radio
  if(!init_Radio()){
    // error
    while(1) {
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);
      delay(250);
    }
  }


  if (PRINT_SERIAL){
    Serial.print(F("Station #"));
    Serial.print(SIMB_ID,DEC);
    Serial.print(F("_"));
    Serial.print(STATION_ID,DEC);
    Serial.println(F(" initialized succesfully."));
  }

  // flash for simb ID
  for (int i=0;i<SIMB_ID;i++) {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(750);
    digitalWrite(LED_BUILTIN,LOW);
    delay(750);
  }

  delay(2000);

  // flash for station ID
  for (int i=0;i<STATION_ID;i++) {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(750);
    digitalWrite(LED_BUILTIN,LOW);
    delay(750);
  }


  // set this false on startup so that any time we black out we reattempt sync
  syncedWithServer = false;

  // start listening for radio traffic
  radio.startReceive();
}

// ******************************* main loop ******************************* //
void loop() {

  // read the pinger data before anything else to avoid serial interference
  volatile uint16_t pingerData = readPinger();

  // flash the light
  for (int i=0;i<STATION_ID;i++){
    digitalWrite(LED_BUILTIN,HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN,LOW);
    delay(500);
  }

  if (PRINT_SERIAL){
    // make sure we're on the right serial pins
    Serial.pins(24,25);
    // begin our serial
    Serial.begin(9600);
    Serial.println(F("Just woke up! Attempting to sync with server..."));
  }


  // variable to check if we've eplictly synced
  volatile bool explicitSync = false;
  // while we're not synced up with the server
  while (!syncedWithServer) {
    // attempt to sync up with the server
    explicitSync = attemptSyncWithServer();
  }

  // set the next sampling alarm
  setSleepAlarm(SAMPLING_INTERVAL_MIN);

  if (PRINT_SERIAL){
    // print hard/soft sync
    if (explicitSync) {
      Serial.println(F("Explicitly synced with server"));
    } else {
      Serial.println(F("Implicitly synced with server"));
    }
  }


  if (PRINT_SERIAL){
    // print off the pinger data
    Serial.print(F("pinger data: "));
    Serial.println(pingerData);
  }

  float tempData[NUM_TEMP_SENSORS];
  // if there are any temp sensors
  if (NUM_TEMP_SENSORS > 0){
    // read them
    readTemps(tempData);

    if (PRINT_SERIAL){
      // print out the temp data
      Serial.println(F("temp data: "));
      for (int i=0;i<NUM_TEMP_SENSORS;i++) {
        Serial.println(tempData[i],3);
      }
    }
  }

  // get the battery voltage
  float voltage = readBatteryVoltage();
  if (PRINT_SERIAL){
    Serial.print(F("voltage: "));
    Serial.println(voltage,3);
  }

  // allocate a buffer
  volatile uint8_t dataBuffer[CLIENT_DATA_SIZE];

  // and pack it
  packClientData(tempData, pingerData, voltage, dataBuffer);

  if (PRINT_SERIAL){
    Serial.println(F("Here's the data we're sending: "));
    for (int i=0;i<CLIENT_DATA_SIZE;i++){
      Serial.print(F(" 0x"));
      Serial.print(dataBuffer[i],HEX);
    }
    Serial.println("");
  }

  // send the data
  if (PRINT_SERIAL){
    Serial.println(F("Attempting radio transmit"));
  }
  int transmitErrorState = sendData_fromClient(dataBuffer,CLIENT_DATA_SIZE);

  if (PRINT_SERIAL){
    if(transmitErrorState == 0){
      Serial.println(F("Successful transmit"));
    } else {
      Serial.print(F("Failed transmit, error "));
      Serial.println(transmitErrorState);
    }
  }

  // if we've exceeded the number of allowable failed transmits
  if (nFailedTransmits >= MAX_FAILED_TRANSMITS_TO_SYNC) {
    // reset our sync flag
    syncedWithServer = false;
  }

  // if we're synced up with the server
  if (syncedWithServer) {

    if (PRINT_SERIAL){
      Serial.print(F("Going to sleep for "));
      Serial.print(SAMPLING_INTERVAL_MIN,DEC);
      Serial.println(F(" minutes"));
    }

    // put the radio to sleep
    radio.sleep();

    delay(100);

    // go to sleep
    LowPower.standby();
  }
}
