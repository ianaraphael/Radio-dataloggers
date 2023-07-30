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


#define IRIDIUM_ENABLE           true // deactivate for testing


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
}



void loop() {

  // TODO: get the time from iridium

  if (justWokeUp) {
    Serial.begin(9600);
    delay(7000);
    Serial.println("Just woke up");
    digitalWrite(13, HIGH);
    justWokeUp = false;
  }

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

    // if we're not already synced with the synced with the network
    if (synchronizedWithNetwork == false) {

      // do that
      sc_RTC.setTime(0,0,0);

      // and set the flag
      synchronizedWithNetwork = true;
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

  // TODO: wrap this in statement below

  // Transmit over Iridium
  if(IRIDIUM_ENABLE) {

    // write the data to the message
    for (int i=0;i<SIMB_DATASIZE;i++){
      message.data[i] = simbData[i];
    }

    // write the timestamp to the message
    message.timestamp = sc_RTC.getEpoch();

    Serial.println("\nTransmitting on Iridium...");
    iridiumOn();
    Serial.println("Iridium is on.");
    sendIridium();
    iridiumOff();
    Serial.println("Done transmitting...");
    Serial.print("Iridium status: ");
    Serial.println(iridiumError);
  }

  // now reset the buffer to error vals
  maskSimbData(simbData);

  // clear the sbd message
  clearMessage();

  // ********** set sleep alarm ********** //

  // if we're synchronized with the network
  if (synchronizedWithNetwork) {

    // if we've been awake for long enough
    if (sc_RTC.getMinutes() < 30 && sc_RTC.getMinutes() >= ceil(SERVER_WAKE_DURATION/2)) {

      Serial.println("Going to sleep");

      // schedule the next sleep alarm
      setAlarm_server();

      digitalWrite(13, LOW);

      // and go to sleep
      sc_RTC.standbyMode();
    }
  }
}
