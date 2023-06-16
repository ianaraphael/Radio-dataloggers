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

#define SIMB_DATASIZE 74
// declare a buffer to hold simb data
uint8_t simbData[SIMB_DATASIZE];

#include "SnowTATOS.h"
#include "SnowTATOS_i2c.h"

#define Serial SerialUSB // comment if not using rocketscream boards

void setup() {

  boardSetup();

  // Begin serial comms
  Serial.begin(9600);

  // init the radio
  init_Radio();

  // init the realtime clock
  init_RTC();

  // init i2c comms with the simb
  init_I2C_scSide();

  // print success
  SerialUSB.println("Server init success");

  // wipe the simb buffer
  memset(simbData,0, sizeof(simbData));

  // and paint error vals in
  maskSimbData(simbData);
}



void loop() {

  // ********** radio comms with clients ********** //

  // allocate a buffer to hold data from a client
  uint8_t currData[CLIENT_DATA_SIZE+4];

  // check for a radio transmission from a client
  int stnID = receiveData_fromClient(currData);

  // if we got data
  if (stnID != -1) {

    // find the start byte in the simbbuffer for this station
    int startByte = (stnID-1)*CLIENT_DATA_SIZE;

    // write the data to the buffer
    for (int i = startByte;i<startByte+7;i++) {
      simbData[i] = currData[i-startByte];
    }

    // if we're not already synced with the synced with the network
    // do that
    rtc.SetTime(0,0,0);
    synchronizedWithNetwork = true;
  }




  // ********** i2c comms with simb ********** //

  // if simb asked for data
  if (simbRequestedData()) {

    // send data to simb
    sendDataToSimb(simbData);

    // now reset the buffer to error vals
    maskSimbData(simbData);
  }




  // ********** set sleep alarm ********** //

  if (synchronizedWithNetwork) {

    // if we've been awake for long enough
    if (rtc.getMinutes() <30 && rtc.getMinutes() >= ceil(SERVER_WAKE_DURATION/2)) {

      // schedule the next sleep alarm
      setAlarm_server();

      // and go to sleep
      rtc.standbyMode();
    }
  }

}
