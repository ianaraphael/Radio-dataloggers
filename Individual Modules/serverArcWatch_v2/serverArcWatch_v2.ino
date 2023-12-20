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

// declare a buffer to hold simb data
uint8_t simbData[SIMB_DATASIZE];


// ********************************* setup ********************************* //
void setup() {

  // Begin serial comms
  Serial.swap(1);
  Serial.begin(9600);
  delay(10000);

  boardSetup();

  // init the radio
  // init_Radio();

  // init the realtime counter
  init_RTC();

  // wipe the simb buffer
  memset(simbData,0, sizeof(simbData));

  // and paint error vals in
  maskSimbData(simbData);

  init_I2C_scSide();

  // print success
  Serial.println("Server init success");
}


// ******************************* main loop ******************************* //
void loop() {

  if (justWokeUp) {

    setWakeAlarm(SERVER_WAKE_DURATION);

    // TODO: radio?
    // pinMode(4, OUTPUT);
    // digitalWrite(4, HIGH);
  }

  if (!timeToSleep) {

    // allocate a broadcast buffer
    int broadcastBuffer[2];

    // put our address in it
    broadcastBuffer[0] = SERVER_ADDRESS;

    // // broadcast that we're awake
    // sendtoWait(broadcastBuffer,sizeof(broadcastBuffer),RH_BROADCAST_ADDRESS);
    //
    // // ************************ client radio comms ************************ //
    //
    // // allocate a buffer to hold data from a client
    // uint8_t currData[CLIENT_DATA_SIZE];
    //
    // // check for a radio transmission from a client
    // int stnID = receiveData_fromClient(currData);
    //
    // // if we got data
    // if (stnID != -1) {
    //
    //   Serial.print("Received data from stn ");
    //   Serial.println(stnID);
    //   Serial.println("");
    //
    //   Serial.println("Raw data: ");
    //   for (int i=0; i<CLIENT_DATA_SIZE;i++){
    //     Serial.print(" 0x");
    //     Serial.print(currData[i],HEX);
    //   }
    //   Serial.println("");
    //
    //   // find the start byte in the simb buffer for this station
    //   int startByte = (stnID-1)*CLIENT_DATA_SIZE;
    //
    //   // write the data to the buffer
    //   for (int i = startByte;i<startByte+CLIENT_DATA_SIZE;i++) {
    //     simbData[i] = currData[i-startByte];
    //   }
    //
    //   float temps[3];
    //   unpackTempData(simbData,temps,stnID);
    //   uint8_t pingerValue = unpackPingerData(simbData,stnID);
    //
    //   Serial.println("  Temps:");
    //   for (int i=0; i<3;i++){
    //     Serial.println(temps[i],3);
    //   }
    //   Serial.println("  Pinger:");
    //   Serial.println(pingerValue);
    // }
  }


  // ***************************** SIMB comms ***************************** //

  // if the simb has requested data
  if (simbRequestedData()) {
    // send the data
    sendDataToSimb(simbData);
  }


  // ******************************* sleep ******************************* //

  // if it's time to go to sleep
  if (timeToSleep) {

    // set the alarm
    setSleepAlarm(SAMPLING_INTERVAL_MIN);

    // and go to sleep
    LowPower.standby();
  }
}
