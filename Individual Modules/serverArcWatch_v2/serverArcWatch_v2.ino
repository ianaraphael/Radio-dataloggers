/*

server_ArcWatch_v2

bytestream based server for ArcWatch deployment. Eliminates local files, human
readable text.

updated for new ultrascream board

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.12.01

*/


// **************************** defines, macros **************************** //
#define Serial Serial2 // comment if not using rocketscream boards

#include "SnowTATOS.h"
#include "SnowTATOS_i2c.h"

// declare a buffer to hold simb data. make it volatile to force retrieval from mem
volatile uint8_t simbData[SIMB_DATASIZE];

#if (STATION_ID != 0)
#error Server STATION_ID must be 0!
#endif



// ********************************* setup ********************************* //
void setup() {

  boardSetup(unusedPins_server);

  if (PRINT_SERIAL) {
    // Begin serial comms
    Serial.swap(1);
    Serial.begin(9600);
    delay(10000);
  }

  // init the realtime counter
  init_RTC();

  // init the radio
  if(!init_Radio()) {
    if (PRINT_SERIAL) {
      Serial.println(F("Failed to init radio."));
    }
    while(1) {
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);
      delay(250);
    }
  }

  // wipe the simb buffer
  memset(simbData,0, sizeof(simbData));

  // and paint error vals in
  maskSimbData(simbData);

  init_I2C_scSide();

  if (PRINT_SERIAL) {
    // print success
    Serial.print(F("Server #"));
    Serial.print(SIMB_ID,DEC);
    Serial.println(F(" initialized successfully"));
  }
}


// ******************************* main loop ******************************* //
void loop() {

  if (PRINT_SERIAL) {
    Serial.swap(1);
    Serial.begin(9600);
    delay(5000);
  }

  // if we're not supposed to be sleeping
  if (!timeToSleep) {

    // if we just woke up
    if (justWokeUp) {

      if (PRINT_SERIAL) {
        Serial.println(F("Just woke up! Broadcasting sync message to network..."));
      }

      // set alarm for duration to stay awake
      setWakeAlarm(SERVER_WAKE_DURATION);

      // allocate a broadcast buffer
      uint8_t broadcastBuffer[3];

      // put our address and sync_term in it
      broadcastBuffer[0] = (uint8_t) SIMB_ID;
      broadcastBuffer[1] = (uint8_t) SERVER_ADDRESS;
      broadcastBuffer[2] = (uint8_t) SYNC_TERM;

      // broadcast that we're awake
      sendPacketTimeout(broadcastBuffer,sizeof(broadcastBuffer));

      // start listening for radio traffic
      radio.startReceive();

      if (PRINT_SERIAL) {
        Serial.println(F("Listening for network radio traffic."));
      }
    }

    // ************************ client radio comms ************************ //

    // allocate a buffer to hold data from a client
    uint8_t currData[CLIENT_DATA_SIZE+2];

    // check for a radio transmission from a client
    int stnID = receiveData_fromClient(currData);

    // if we got data
    if (stnID != -1) {

      if (PRINT_SERIAL) {
        Serial.print(F("Received data from stn "));
        Serial.println(stnID);
        Serial.println("");

        Serial.println(F("Raw data: "));
        for (int i=0; i<CLIENT_DATA_SIZE;i++){
          Serial.print(" 0x");
          Serial.print(currData[i],HEX);
        }
        Serial.println(F(""));
      }

      // find the start byte in the simb buffer for this station
      int startByte = (stnID-1)*CLIENT_DATA_SIZE;

      // write the data to the buffer
      for (int i = startByte;i<startByte+CLIENT_DATA_SIZE;i++) {
        simbData[i] = currData[i-startByte];
      }

      // if there are any temp sensors
      if (NUM_TEMP_SENSORS > 0){

        // unpack and print the temp data
        float temps[NUM_TEMP_SENSORS];
        unpackTempData(simbData,temps,stnID);
        if (PRINT_SERIAL) {
          Serial.println(F("  Temps:"));
          for (int i=0; i<NUM_TEMP_SENSORS;i++){
            Serial.println(temps[i],3);
          }
        }
      }

      // unpack and print the pinger data
      uint16_t pingerValue = unpackPingerData(simbData,stnID);
      if (PRINT_SERIAL) {
        Serial.print(F("  Pinger: "));
        Serial.println(pingerValue);
      }

      // unpack and print the voltage
      float voltage = unpackVoltageData(simbData,stnID);
      if (PRINT_SERIAL) {
        Serial.print(F("  Voltage: "));
        Serial.println(voltage,1); // 1 decimal place
      }
    }
  }


  // ***************************** SIMB comms ***************************** //

  // if the simb has requested data
  if (simbRequestedData()) {
    delay(500);
    digitalWrite(LED_BUILTIN,LOW);
    // send the data
    sendDataToSimb(simbData);
  }


  // ******************************* sleep ******************************* //

  // if it's time to go to sleep
  if (timeToSleep) {

    // if the alarm isn't already activated
    if (!sleepAlarmActivated) {

      // set the alarm
      setSleepAlarm(SAMPLING_INTERVAL_MIN);

      if (PRINT_SERIAL) {
        Serial.print(F("Going to sleep for "));
        Serial.print(SAMPLING_INTERVAL_MIN-SERVER_WAKE_DURATION,DEC);
        Serial.println(F(" minutes"));
      }
    }

    // shut down the radio
    radio.sleep();

    // // Serial0.end();
    // pinMode(0,OUTPUT);
    // digitalWrite(0,LOW);
    // pinMode(1,OUTPUT);
    // digitalWrite(1,LOW);
    //
    // Serial1.end();
    // pinMode(8,OUTPUT);
    // digitalWrite(8,LOW);
    // pinMode(9,OUTPUT);
    // digitalWrite(9,LOW);
    //
    // Serial2.end();
    // pinMode(24,OUTPUT);
    // digitalWrite(24,LOW);
    // pinMode(25,OUTPUT);
    // digitalWrite(25,LOW);

    delay(1000);

    // and go to sleep
    LowPower.standby();
  }
}
