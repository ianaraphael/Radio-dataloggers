/*

client_ArcWatch_v1

bytestream based client for ArcWatch deployment. Eliminates local files, human
readable text

Ian Raphael
ian.a.raphael.th@dartmouth.edu
2023.06.16

*/

#include "SnowTATOS.h"

#define Serial SerialUSB // comment if not using rocketscream boards

#define CLIENT_TRANSMIT_DELAY_SECS 10 // wait a few seconds after waking before trying to transmit to server

void setup() {

  boardSetup();

  // Begin serial comms
  Serial.begin(9600);
  delay(5000);

  // init the temperature sensors
  initTemps();

  // init the realtime clock
  init_RTC();

  Serial.print("init time (hours, min, sec): ");
  Serial.print(sc_RTC.getHours(),DEC);
  Serial.print(", ");
  Serial.print(sc_RTC.getMinutes(),DEC);
  Serial.print(", ");
  Serial.println(sc_RTC.getSeconds(),DEC);

  // init the radio
  init_Radio();

  Serial.print("Station #");
  Serial.print(STATION_ID,DEC);
  Serial.println(" init'd");

  // if it's not a test
  if (!TEST) {

    // we're going to sleep
    Serial.println("Going to sleep");

    bool firstAlarm = true;

    // set the first alarm for midnight
    setAlarm(firstAlarm);
    // digitalWrite(13,LOW);

    Serial.print("First alarm set for (hour, min): ");
    Serial.print(ALARM_HOURS,DEC);
    Serial.print(", ");
    Serial.println(ALARM_MINUTES,DEC);

    delay(1000);

    // and go to sleep
    sc_RTC.standbyMode();

  }
}



void loop() {

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  init_Radio();

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

  // if we've been awake for long enough (or testing)
  int elapsedTime_secs = ((millis()-wokeUpAtMillis)/1000);
  if ((elapsedTime_secs > CLIENT_TRANSMIT_DELAY_SECS) or TEST) {

    // send the data
    Serial.println("Attempting radio transmit");

    if(manager.sendtoWait(dataBuffer,CLIENT_DATA_SIZE, SERVER_ADDRESS)){
      Serial.println("Successful transmit");
    } else {
      Serial.println("Failed transmit");
    }

    // if it's not a test, set the alarm and go to sleep
    if (!TEST) {
      bool firstAlarm = false;

      // set the alarm
      setAlarm(firstAlarm);

      Serial.print("Next alarm set for (hour, min): ");
      Serial.print(ALARM_HOURS,DEC);
      Serial.print(", ");
      Serial.println(ALARM_MINUTES,DEC);

      delay(1000);

      // digitalWrite(13,LOW);

      // go to sleep
      sc_RTC.standbyMode();

    } else {
      delay(2000);
    }
  }
}
