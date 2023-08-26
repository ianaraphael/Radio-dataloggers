// server

/*

server_ArcWatch_v2

bytestream based server for ArcWatch deployment. Eliminates local files, human
readable text.

standalone iridium service

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.07.27

*/

#define Serial SerialUSB // comment if not using rocketscream boards

#include "SnowTATOS.h"
#include "SnowTATOS_i2c.h"

// declare a buffer to hold simb data (we're actually just going to use this to transfer via iridium)
uint8_t simbData[SIMB_DATASIZE];

void setup() {

  // Begin serial comms
  Serial.begin(9600);
  delay(10000);

  boardSetup();

  // init the radio
  init_Radio();

  // init the realtime clock
  init_RTC();

  // wipe the simb buffer
  memset(simbData,0, sizeof(simbData));

  // and paint error vals in
  maskSimbData(simbData);

  // print success
  Serial.println("Server init success");

  // if it's not a test
  if (!TEST) {

    // we're going to sleep
    Serial.println("Going to sleep");

    bool firstAlarm=true;
    // set the first alarm for midnight
    setAlarm(firstAlarm);

    // and go to sleep
    sc_RTC.standbyMode();
  }
}



void loop() {

  // init the radio
  init_Radio();

  // ********** radio comms with clients ********** //

  // allocate a buffer to hold data from a client (with some padding)
  uint8_t currData[CLIENT_DATA_SIZE];

  // check for a radio transmission from a client
  int stnID = receiveData_fromClient(currData);

  // if we got data
  if (stnID != -1) {

    Serial.print("Received data from stn ");
    Serial.println(stnID);
    Serial.println("");

    Serial.println("Raw data: ");
    for (int i=0; i<CLIENT_DATA_SIZE;i++){
      Serial.print(" 0x");
      Serial.print(currData[i],HEX);
    }
    Serial.println("");

    // find the start byte in the simb buffer for this station
    int startByte = (stnID-1)*CLIENT_DATA_SIZE;

    // write the data to the buffer
    for (int i = startByte;i<startByte+CLIENT_DATA_SIZE;i++) {
      simbData[i] = currData[i-startByte];
    }

    float temps[3];
    unpackTempData(simbData,temps,stnID);
    uint8_t pingerValue = unpackPingerData(simbData,stnID);

    Serial.println("  Temps:");
    for (int i=0; i<3;i++){
      Serial.println(temps[i],3);
    }
    Serial.println("  Pinger:");
    Serial.println(pingerValue);
  }


  // ********** iridium transmit and sleep ********** //

  // if we've been awake for long enough (or we're testing)
  int elapsedTime_mins = (millis() - wokeUpAtMillis)/60000;
  if ((elapsedTime_mins >= SERVER_WAKE_DURATION) or TEST) {

    // write the data to the message
    for (int i=0;i<SIMB_DATASIZE;i++){
      message.data[i] = simbData[i];
    }

    // write the timestamp to the message
    message.timestamp = sc_RTC.getEpoch();

    // if iridium is enabled
    if (IRIDIUM_ENABLE) {

      // Transmit over Iridium
      Serial.println("\nTransmitting on Iridium...");

      // turn the iridium unit on
      iridiumOn();
      Serial.println("Iridium is on.");

      // send the data
      sendIridium();

      // turn the iridium unit off
      iridiumOff();
      Serial.println("Done transmitting...");

      // print error status
      Serial.print("Iridium status: ");
      Serial.println(iridiumError);

      // now reset the buffer to error vals
      maskSimbData(simbData);

      // clear the sbd message
      clearMessage();

      // we're going to sleep
      Serial.println("Going to sleep");

      bool firstAlarm = false;
      // schedule the next sleep alarm normally
      setAlarm(firstAlarm);

      // and go to sleep
      sc_RTC.standbyMode();

      // otherwise
    } else {

      // print the message that we would send
      Serial.println("Here's what we would send on iridium: ");
      for (int i=0;i<sizeof(message);i++){
        Serial.print(message.bytes[i],HEX);
        Serial.print(" ");
      }

      delay(5000);
      Serial.println("online");
    }
  }
}
