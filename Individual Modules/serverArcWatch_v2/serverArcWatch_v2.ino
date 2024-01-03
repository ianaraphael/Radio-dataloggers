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

#if (STATION_ID != 0)
  #error Server STATION_ID must be 0!
#endif



// ********************************* setup ********************************* //
void setup() {

  boardSetup();

  // Begin serial comms
  Serial.swap(1);
  Serial.begin(9600);
  delay(10000);

  // init the realtime counter
  init_RTC();

  // init the radio
  init_Radio();

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

    Serial.println("just woke up!");

    setWakeAlarm(SERVER_WAKE_DURATION);

    // do a broadcast

    // allocate a broadcast buffer
    uint8_t broadcastBuffer[2];

    // put our address in it
    broadcastBuffer[0] = (uint8_t) SERVER_ADDRESS;
    broadcastBuffer[1] = (uint8_t) SERVER_ADDRESS;

    // broadcast that we're awake
    sendPacketTimeout(broadcastBuffer,sizeof(broadcastBuffer));

    // start listening for radio traffic
    radio.startReceive();
  }


  if (!timeToSleep) {

    // ************************ client radio comms ************************ //

    // allocate a buffer to hold data from a client
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

      // if there are any temp sensors
      if (NUM_TEMP_SENSORS > 0){

        // unpack and print the temp data
        float temps[NUM_TEMP_SENSORS];
        unpackTempData(simbData,temps,stnID);
        Serial.println("  Temps:");
        for (int i=0; i<NUM_TEMP_SENSORS;i++){
          Serial.println(temps[i],3);
        }
      }

      // unpack and print the pinger data
      uint16_t pingerValue = unpackPingerData(simbData,stnID);
      Serial.print("  Pinger: ");
      Serial.println(pingerValue);

      // unpack and print the voltage
      float voltage = unpackVoltageData(simbData,stnID);
      Serial.print("  Voltage: ");
      Serial.println(voltage,2);
    }
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

    // shut down the radio
    radio.sleep();

    // set the alarm
    setSleepAlarm(SAMPLING_INTERVAL_MIN);

    // and go to sleep
    LowPower.standby();
  }
}
