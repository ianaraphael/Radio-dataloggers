// server


// setup

// board setup
// start radio
// start rtc
// begin wire

/*

server_ArcWatch_v1

bytestream based server for ArcWatch deployment. Eliminates local files, human
readable text

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.06.16

*/

#define Serial SerialUSB // comment if not using rocketscream boards

#include "SnowTATOS.h"
#include "SnowTATOS_i2c.h"

// declare a buffer to hold simb data
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

  // init i2c comms with the simb
  init_I2C_scSide();

  // print success
  Serial.println("Server init success");
}



void loop() {

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




  // ********** i2c comms with simb ********** //

  // if simb asked for data
  if (simbRequestedData()) {

    // send data to simb
    sendDataToSimb(simbData);

    Serial.println("Sent data to simb: ");
    for (int i=0;i<SIMB_DATASIZE;i++){
      Serial.print(" ");
      Serial.print(simbData[i],HEX);
    }
    Serial.println(" ");

    // now reset the buffer to error vals
    maskSimbData(simbData);
  }

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
